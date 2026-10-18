#include "updater.hpp"
#include "updatedialog.hpp"
#include "updatecommit.hpp"
#include "appimageupdate.hpp"
#include "language.hpp"
#include <QApplication>
#include <QWidget>
#include <QProcess>
#include <QProcessEnvironment>
#include <QSysInfo>
#include <QNetworkReply>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QSaveFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QStandardPaths>
#include <QVersionNumber>
#include <QRegularExpression>
#include <QSettings>
#include <QDateTime>
#include <QTimer>
#include <QEventLoop>
#include <QElapsedTimer>
#include <QLockFile>
#include <QUuid>
namespace {
QString t(const char *s) {
    return Language::text(QString::fromUtf8(s));
}
QNetworkRequest request(const QString &url) {
    QNetworkRequest r{QUrl(url)};
    r.setRawHeader("User-Agent", "ThinkCad/" THINKERCAD_VERSION);
    r.setTransferTimeout(30000);
    r.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    return r;
}
bool allowed(const QUrl &url) {
    return url.scheme() == "https" && (url.host() == "api.github.com" || url.host() == "github.com" ||
                                       url.host() == "release-assets.githubusercontent.com" ||
                                       url.host() == "objects.githubusercontent.com");
}
} // namespace
Updater::Updater(QObject *parent) : QObject(parent), network_(this), download_(this) {
    appImagePath_ = QFileInfo(qEnvironmentVariable("APPIMAGE")).canonicalFilePath();
#ifndef Q_OS_WIN
    packageManaged_ = appImagePath_.isEmpty();
#endif
    const auto logs =
        QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/updates/logs";
    QDir().mkpath(logs);
    log_ = logs + "/" + QUuid::createUuid().toString(QUuid::WithoutBraces) + ".log";
    QSettings receipt(QCoreApplication::applicationDirPath() + "/thinkcad-update-receipt.ini",
                      QSettings::IniFormat);
    if (receipt.value("version").toString() == THINKERCAD_VERSION &&
        receipt.value("status").toString() == "success") {
        QSettings settings;
        for (const auto &key : {"version", "date", "path", "seconds"})
            settings.setValue("updates/installed/" + QString(key), receipt.value(key));
    }
    connect(&download_, &UpdateDownload::progress, this, &Updater::transferProgress);
    connect(&download_, &UpdateDownload::stateChanged, this, [this](UpdateDownload::State state) {
        using S = UpdateDownload::State;
        busy_ = state == S::Downloading || state == S::Retrying || state == S::Hashing || state == S::Paused;
        if (state == S::Downloading)
            report(t("Update herunterladen"));
        else if (state == S::Paused)
            report(t("Download pausiert. Die Teildatei bleibt lokal gespeichert."));
        else if (state == S::Retrying)
            report(t("Verbindung unterbrochen. Begrenzter Wiederholungsversuch…"));
        else if (state == S::Hashing)
            report(t("SHA-256-Prüfung läuft…"));
        else if (state == S::Cancelled)
            report(t("Download abgebrochen."));
        emit changed();
    });
    connect(&download_, &UpdateDownload::failed, this, [this](const QString &reason) {
        hashValid_ = false;
        hashFailed_ = reason == "SHA-256";
        failure(t("Download oder Prüfsumme ungültig. Keine Installation."),
                reason + "\n" +
                    t("Verbindung, freien Speicher und Schreibrechte prüfen. Bei einer ungültigen Prüfsumme "
                      "erneut herunterladen."));
    });
    connect(&download_, &UpdateDownload::verified, this, [this](const QString &path) {
        hashValid_ = true;
        verifyDownload(path, version());
    });
}
Updater::~Updater() {
    for (auto *reply : network_.findChildren<QNetworkReply *>()) {
        disconnect(reply, nullptr, this, nullptr);
        reply->abort();
    }
    for (auto *process : findChildren<QProcess *>()) {
        disconnect(process, nullptr, this, nullptr);
        if (process->state() != QProcess::NotRunning) {
            process->kill();
            process->waitForFinished(3000);
        }
    }
}
void Updater::cancelDownload(bool keep) {
    ++generation_;
    for (auto *reply : network_.findChildren<QNetworkReply *>()) {
        disconnect(reply, nullptr, this, nullptr);
        reply->abort();
        reply->deleteLater();
    }
    for (auto *process : findChildren<QProcess *>())
        if (process->state() != QProcess::NotRunning)
            process->kill();
    download_.cancel(keep);
    installer_.clear();
    hashValid_ = false;
    busy_ = false;
    if (!keep && !download_.destination().isEmpty()) {
        QFile::remove(download_.destination());
        QFile::remove(download_.destination() + ".sigstore.jsonl");
    }
    report(t("Download abgebrochen."));
}
void Updater::report(const QString &status, const QString &detail) {
    status_ = status;
    details_ = detail;
    QFile log(log_);
    if (log.open(QIODevice::WriteOnly | QIODevice::Append) && log.size() < 5 * 1024 * 1024)
        log.write((QDateTime::currentDateTimeUtc().toString(Qt::ISODate) + " " + status + " " + detail + "\n")
                      .toUtf8());
    emit message(status + (detail.isEmpty() ? QString() : " " + detail));
    emit changed();
}
void Updater::failure(const QString &step, const QString &detail) {
    busy_ = false;
    installer_.clear();
    report(step, detail);
}
void Updater::showDialog() {
    if (!dialog_)
        dialog_ = new UpdateDialog(*this, qobject_cast<QWidget *>(parent()));
    dialog_->show();
    dialog_->raise();
    dialog_->activateWindow();
    if (!busy_ && !ready())
        performCheck(true);
}
void Updater::check() {
    if (QSettings().value("updates/automatic", true).toBool())
        performCheck(false);
}
void Updater::fetch(const QString &url, qint64 limit, std::function<void(QByteArray)> callback, bool sha,
                    int retry) {
    const auto generation = generation_;
    auto r = request(url);
    if (sha)
        r = thinkcad_update::commitRequest(r);
    auto *reply = network_.get(r);
    auto bytes = std::make_shared<QByteArray>();
    auto overflow = std::make_shared<bool>(false);
    connect(reply, &QNetworkReply::readyRead, this, [reply, bytes, overflow, limit] {
        if (bytes->size() + reply->bytesAvailable() > limit) {
            *overflow = true;
            reply->abort();
            return;
        }
        bytes->append(reply->readAll());
    });
    connect(reply, &QNetworkReply::finished, this,
            [this, reply, bytes, overflow, limit, url, callback, sha, retry, generation] {
                reply->deleteLater();
                if (generation != generation_)
                    return;
                if (bytes->size() + reply->bytesAvailable() > limit)
                    *overflow = true;
                else
                    bytes->append(reply->readAll());
                const int code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
                if (!*overflow && allowed(reply->url()) && reply->error() == QNetworkReply::NoError &&
                    code == 200) {
                    callback(*bytes);
                    return;
                }
                if (!*overflow && (code == 0 || code == 429 || code >= 500) && retry < 3) {
                    report(t("Verbindung unterbrochen. Begrenzter Wiederholungsversuch…"));
                    QTimer::singleShot(1000 * (1 << retry), this, [=, this] {
                        if (generation == generation_)
                            fetch(url, limit, callback, sha, retry + 1);
                    });
                    return;
                }
                failure(t("Update-Metadaten konnten nicht geprüft werden."),
                        QString("HTTP %1: %2").arg(code).arg(reply->errorString()) + "\n" +
                            t("Verbindung prüfen und die Update-Prüfung erneut starten."));
            });
}
void Updater::performCheck(bool manual) {
    if (busy_ || ready())
        return;
    ++generation_;
    available_ = false;
    url_.clear();
    size_ = 0;
    hashValid_ = false;
    hashFailed_ = attestationFailed_ = false;
    release_ = {};
    busy_ = true;
    report(t("Update prüfen…"));
    fetch(
        "https://api.github.com/repos/Lyvt-Dev/ThinkCad/releases/latest", 1048576,
        [this, manual](QByteArray bytes) {
            auto release = QJsonDocument::fromJson(bytes).object();
            const auto m =
                QRegularExpression("^v([0-9]+\\.[0-9]+\\.[0-9]+)$").match(release["tag_name"].toString());
            if (!m.hasMatch() || release["draft"].toBool() || release["prerelease"].toBool()) {
                failure(t("Kein gültiges stabiles Release gefunden."), {});
                return;
            }
            release_ = release;
            busy_ = false;
            if (QVersionNumber::fromString(m.captured(1)) <= QVersionNumber::fromString(THINKERCAD_VERSION)) {
                report(t("ThinkCad ist aktuell."));
                return;
            }
            available_ = true;
            if (!manual && QSettings().value("updates/skippedVersion").toString() == version())
                return;
#ifdef Q_OS_WIN
            filename_ = "ThinkCad-Setup-" + m.captured(1) + ".exe";
#else
            filename_ = "ThinkCad-" + m.captured(1) + "-x86_64.AppImage";
            if (!packageManaged_ && QSysInfo::currentCpuArchitecture() != "x86_64") {
                failure(t("AppImage-Updates benötigen x86_64 Linux."), {});
                return;
            }
#endif
            url_.clear();
            checksumUrl_.clear();
            attestationUrl_.clear();
            for (const auto &item : release["assets"].toArray()) {
                const auto a = item.toObject();
                const auto name = a["name"].toString();
                const auto url = a["browser_download_url"].toString();
                if (!url.startsWith("https://github.com/Lyvt-Dev/ThinkCad/releases/download/" + version() +
                                    "/"))
                    continue;
                if (name == filename_ && a["size"].toDouble() > 0 &&
                    a["size"].toDouble() < 512ll * 1024 * 1024) {
                    url_ = url;
                    size_ = qint64(a["size"].toDouble());
                }
                if (name == filename_ + ".sha256")
                    checksumUrl_ = url;
                if (name == filename_ + ".sigstore.jsonl" && a["size"].toDouble() < 5 * 1024 * 1024)
                    attestationUrl_ = url;
            }
            if (!packageManaged_ && (url_.isEmpty() || checksumUrl_.isEmpty() || attestationUrl_.isEmpty())) {
                failure(t("Release enthält keinen Installer mit Prüfsumme und Buildnachweis."), {});
                return;
            }
            report(t("Neue Version verfügbar. Release-Notes vor dem Download prüfen."));
            if (!dialog_)
                dialog_ = new UpdateDialog(*this, qobject_cast<QWidget *>(parent()));
            dialog_->show();
            dialog_->raise();
        });
}
void Updater::skipVersion() {
    if (busy_)
        return;
    QSettings().setValue("updates/skippedVersion", version());
    report(t("Diese Version wird bei automatischen Prüfungen übersprungen."));
    if (dialog_)
        dialog_->hide();
}
void Updater::beginDownload() {
    if (busy_ || ready() || url_.isEmpty() || packageManaged_)
        return;
    busy_ = true;
    hashValid_ = false;
    hashFailed_ = attestationFailed_ = false;
    const auto v = version().mid(1);
    report(t("Release-Commit und Prüfsumme laden…"));
    fetch(
        "https://api.github.com/repos/Lyvt-Dev/ThinkCad/commits/" + version(), 128,
        [this, v](QByteArray bytes) {
            expectedCommit_ = thinkcad_update::commitSha(bytes);
            if (expectedCommit_.isEmpty()) {
                failure(t("Release-Commit konnte nicht geprüft werden."), {});
                return;
            }
            fetch(checksumUrl_, 4096, [this, v](QByteArray bytes) {
                const auto m = QRegularExpression("^([a-fA-F0-9]{64})[ \\t]+\\*?" +
                                                  QRegularExpression::escape(filename_) + "(?:\\r?\\n)?$")
                                   .match(QString::fromLatin1(bytes));
                if (!m.hasMatch()) {
                    failure(t("Ungültige Update-Prüfsumme."), {});
                    return;
                }
                verifiedHash_ = QByteArray::fromHex(m.captured(1).toLatin1());
                directory_ = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/updates/" +
                             v + "-" + QString::fromLatin1(verifiedHash_.toHex());
                if (QFileInfo(directory_).isSymLink() || !QDir().mkpath(directory_)) {
                    failure(t("Update-Verzeichnis ist nicht beschreibbar."), directory_);
                    return;
                }
                fetch(attestationUrl_, 5 * 1024 * 1024, [this](QByteArray bundle) {
                    if (bundle.isEmpty()) {
                        failure(t("Buildnachweis konnte nicht geladen werden."), {});
                        return;
                    }
                    QSaveFile file(directory_ + "/" + filename_ + ".sigstore.jsonl");
                    if (!file.open(QIODevice::WriteOnly) || file.write(bundle) != bundle.size() ||
                        !file.commit()) {
                        failure(t("Buildnachweis konnte nicht gespeichert werden."), file.errorString());
                        return;
                    }
                    download_.start(QUrl(url_), directory_ + "/" + filename_, size_, verifiedHash_);
                });
            });
        },
        true);
}
void Updater::verifyDownload(const QString &path, const QString &v) {
#ifdef Q_OS_WIN
    const QString tool = "gh.exe", workflow = "windows.yml";
#else
    const QString tool = "gh", workflow = "linux.yml";
#endif
    const auto verifier = QCoreApplication::applicationDirPath() + "/verifier/" + tool,
               roots = QCoreApplication::applicationDirPath() + "/verifier/trusted_root.jsonl";
    if (!QFileInfo::exists(verifier) || !QFileInfo::exists(roots)) {
        failure(t("Update-Verifizierer fehlt in dieser Installation."),
                t("Installation reparieren. Keine Installation des Updates."));
        return;
    }
    busy_ = true;
    report(t("Prüfsumme gültig. GitHub-/Sigstore-Buildnachweis wird geprüft…"));
    auto *check = new QProcess(this);
    auto done = std::make_shared<bool>(false);
    auto output = std::make_shared<QByteArray>();
    connect(check, &QProcess::readyReadStandardOutput, this, [check, output] {
        auto b = check->readAllStandardOutput();
        if (output->size() + b.size() > 1048576)
            check->kill();
        else
            output->append(b);
    });
    const auto generation = generation_;
    auto complete = [this, check, done, output, path, v, generation](bool ok) {
        if (*done)
            return;
        *done = true;
        check->deleteLater();
        if (generation != generation_)
            return;
        output->append(check->readAllStandardOutput());
        const auto rows = QJsonDocument::fromJson(*output).array();
        bool valid = ok && !rows.isEmpty();
        for (const auto &row : rows)
            valid = valid && row.toObject()["verificationResult"]
                                     .toObject()["signature"]
                                     .toObject()["certificate"]
                                     .toObject()["sourceRepositoryDigest"]
                                     .toString() == expectedCommit_;
        if (!valid) {
            attestationFailed_ = true;
            failure(t("Buildnachweis ungültig. Keine Installation."),
                    t("Repository, Workflow, Branch oder Quellcommit stimmen nicht überein, oder der "
                      "Verifizierer konnte nicht ausgeführt werden. Update erneut laden oder Installation "
                      "reparieren.") +
                        "\nVerifier exit: " + QString::number(check->exitCode()) + "\n" +
                        QString::fromUtf8(check->readAllStandardError()).left(4096));
            return;
        }
        installer_ = path;
        busy_ = false;
        report(t("Update geprüft und bereit zur Installation."));
        emit updateReady(v);
    };
    connect(check, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
            [complete](int code, QProcess::ExitStatus status) {
                complete(code == 0 && status == QProcess::NormalExit);
            });
    connect(check, &QProcess::errorOccurred, this, [complete](QProcess::ProcessError e) {
        if (e == QProcess::FailedToStart)
            complete(false);
    });
    QTimer::singleShot(60000, check, [check] {
        if (check->state() != QProcess::NotRunning)
            check->kill();
    });
    auto env = QProcessEnvironment::systemEnvironment();
    env.remove("GH_TOKEN");
    env.remove("GITHUB_TOKEN");
    check->setProcessEnvironment(env);
    check->start(verifier,
                 {"attestation", "verify", path, "--bundle", path + ".sigstore.jsonl",
                  "--custom-trusted-root", roots, "--repo", "Lyvt-Dev/ThinkCad", "--signer-workflow",
                  "Lyvt-Dev/ThinkCad/.github/workflows/" + workflow, "--source-ref", "refs/heads/main",
                  "--source-digest", expectedCommit_, "--deny-self-hosted-runners", "--format", "json"});
}
bool Updater::restartAndInstall() {
    if (!ready() || !QFileInfo::exists(installer_)) {
        failure(Language::text("Update ist nicht mehr gültig."), {});
        return false;
    }
#ifdef Q_OS_WIN
    const auto cache = QFileInfo(installer_).absolutePath();
    QDir().mkpath(cache + "/verifier");
    for (const auto &name : QStringList{"gh.exe", "trusted_root.jsonl"}) {
        QFile source(QCoreApplication::applicationDirPath() + "/verifier/" + name);
        QSaveFile target(cache + "/verifier/" + name);
        if (!source.open(QIODevice::ReadOnly) || !target.open(QIODevice::WriteOnly)) {
            failure(Language::text("Update-Verifizierer fehlt."), cache);
            return false;
        }
        const auto bytes = source.readAll();
        if (target.write(bytes) != bytes.size() || !target.commit()) {
            failure(Language::text("Update-Verifizierer fehlt."), target.errorString());
            return false;
        }
    }
    const auto helper =
        cache + "/ThinkCadUpdater-" + QUuid::createUuid().toString(QUuid::WithoutBraces) + ".exe";
    if (!QFile::copy(QCoreApplication::applicationDirPath() + "/ThinkCadUpdater.exe", helper)) {
        failure(Language::text("Update-Helfer konnte nicht bereitgestellt werden."), cache);
        return false;
    }
    if (!QProcess::startDetached(helper, {QString::number(QCoreApplication::applicationPid()), installer_,
                                          QCoreApplication::applicationDirPath(), expectedCommit_,
                                          version().mid(1), Language::current()})) {
        QFile::remove(helper);
        failure(Language::text("Update-Helfer konnte nicht gestartet werden."), helper);
        return false;
    }
    return true;
#elif defined(Q_OS_LINUX)
    if (appImagePath_.isEmpty()) {
        emit message("Use your package manager for this installation, or run the portable AppImage.");
        return false;
    }
    QLockFile transactionLock(appImagePath_ + ".transaction-lock");
    if (!transactionLock.tryLock(0)) {
        failure(t("AppImage-Installation fehlgeschlagen."), QString("Another update owns ") + appImagePath_);
        return false;
    }
    QElapsedTimer installationTime;
    installationTime.start();
    busy_ = true;
    report(t("AppImage sichern und atomar ersetzen…"));
    QString error;
    if (!replaceAppImage(appImagePath_, installer_, verifiedHash_, &error, [this](qint64 n, qint64 total) {
            emit transferProgress(n, total, 0, -1);
            QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        })) {
        failure(t("AppImage-Installation fehlgeschlagen."), error);
        return false;
    }
    QProcess launcher;
    auto env = QProcessEnvironment::systemEnvironment();
    for (auto name : {"LD_LIBRARY_PATH", "QT_PLUGIN_PATH", "QT_QPA_PLATFORM_PLUGIN_PATH"}) {
        auto original = "THINKCAD_ORIGINAL_" + QString::fromLatin1(name);
        if (env.contains(original))
            env.insert(name, env.value(original));
        else
            env.remove(name);
    }
    env.remove("APPDIR");
    env.remove("APPIMAGE");
    report(t("Neue Installation wird geprüft…"));
    QProcess health;
    auto healthEnv = env;
    healthEnv.insert("QT_QPA_PLATFORM", "offscreen");
    health.setProcessEnvironment(healthEnv);
    health.start(appImagePath_, {"--smoke-test", "--update-health-check=" + version().mid(1)});
    QElapsedTimer deadline;
    deadline.start();
    while (health.state() != QProcess::NotRunning && deadline.elapsed() < 120000) {
        health.waitForFinished(50);
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    }
    if (health.state() != QProcess::NotRunning) {
        health.kill();
        health.waitForFinished(3000);
    }
    if (health.error() == QProcess::FailedToStart || health.exitStatus() != QProcess::NormalExit ||
        health.exitCode() != 0) {
        const bool restored = restoreAppImage(appImagePath_, &error);
        failure(t("Startprüfung fehlgeschlagen."),
                restored ? t("Vorherige AppImage-Version wiederhergestellt.") : error);
        return false;
    }
    QSettings settings;
    settings.setValue("updates/installed/seconds", installationTime.elapsed() / 1000);
    settings.setValue("updates/installed/version", version().mid(1));
    settings.setValue("updates/installed/date", QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    settings.setValue("updates/installed/path", appImagePath_);
    launcher.setProcessEnvironment(env);
    launcher.setProgram(appImagePath_);
    launcher.setWorkingDirectory(QDir::homePath());
    installer_.clear();
    if (!launcher.startDetached()) {
        emit message("AppImage updated. Close and reopen it to use the new version.");
        return false;
    }
    return true;
#else
    return false;
#endif
}
