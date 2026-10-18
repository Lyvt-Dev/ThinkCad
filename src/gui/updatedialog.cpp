#include "updatedialog.hpp"
#include "updater.hpp"
#include "language.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QCheckBox>
#include <QTextBrowser>
#include <QSettings>
#include <QDateTime>
#include <QLocale>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
namespace {
QString t(const char *s) {
    return Language::text(QString::fromUtf8(s));
}
} // namespace
namespace {
class ReleaseNotesBrowser final : public QTextBrowser {
  protected:
    QVariant loadResource(int, const QUrl &) override {
        return {};
    }
};
} // namespace
UpdateDialog::UpdateDialog(Updater &u, QWidget *p) : QDialog(p), updater_(u) {
    setObjectName("updateDialog");
    setWindowTitle(t("ThinkCad aktualisieren"));
    resize(700, 620);
    auto *layout = new QVBoxLayout(this);
    layout->setSpacing(12);
    release_ = new QLabel;
    release_->setWordWrap(true);
    release_->setTextFormat(Qt::PlainText);
    layout->addWidget(release_);
    status_ = new QLabel;
    status_->setTextFormat(Qt::PlainText);
    status_->setWordWrap(true);
    layout->addWidget(status_);
    progress_ = new QProgressBar;
    progress_->setObjectName("updateProgress");
    progress_->setAccessibleName(t("Update-Fortschritt"));
    layout->addWidget(progress_);
    metrics_ = new QLabel;
    metrics_->setWordWrap(true);
    layout->addWidget(metrics_);
    security_ = new QLabel;
    security_->setTextFormat(Qt::PlainText);
    security_->setWordWrap(true);
    layout->addWidget(security_);
    notes_ = new ReleaseNotesBrowser;
    notes_->setObjectName("updateReleaseNotes");
    notes_->setPlaceholderText(t("Noch nicht geprüft"));
    notes_->setOpenExternalLinks(false);
    notes_->setOpenLinks(false);
    layout->addWidget(notes_, 1);
    auto *automatic = new QCheckBox(t("Automatisch nach Updates suchen"));
    automatic->setChecked(QSettings().value("updates/automatic", true).toBool());
    layout->addWidget(automatic);
    connect(automatic, &QCheckBox::toggled, this,
            [](bool value) { QSettings().setValue("updates/automatic", value); });
    auto *buttons = new QGridLayout;
    download_ = new QPushButton(t("Herunterladen"));
    pause_ = new QPushButton;
    cancel_ = new QPushButton(t("Abbrechen"));
    install_ = new QPushButton(t("Speichern und installieren"));
    skip_ = new QPushButton(t("Diese Version überspringen"));
    download_->setObjectName("updateDownload");
    pause_->setObjectName("updatePause");
    install_->setObjectName("updateInstall");
    for (auto *b : {download_, pause_, cancel_, install_, skip_})
        b->setAutoDefault(false);
    buttons->addWidget(download_, 0, 0);
    buttons->addWidget(pause_, 0, 1);
    buttons->addWidget(cancel_, 0, 2);
    buttons->addWidget(skip_, 1, 0, 1, 2);
    buttons->addWidget(install_, 1, 2);
    layout->addLayout(buttons);
    auto *bottom = new QHBoxLayout;
    auto *log = new QPushButton(t("Update-Protokoll öffnen"));
    auto *close = new QPushButton(t("Schließen"));
    bottom->addWidget(log);
    bottom->addStretch();
    bottom->addWidget(close);
    layout->addLayout(bottom);
    connect(close, &QPushButton::clicked, this, &QDialog::hide);
    connect(log, &QPushButton::clicked, this,
            [this] { QDesktopServices::openUrl(QUrl::fromLocalFile(updater_.logPath())); });
    connect(download_, &QPushButton::clicked, &u, &Updater::beginDownload);
    connect(pause_, &QPushButton::clicked, this, [this] {
        if (updater_.downloadState() == UpdateDownload::State::Paused)
            updater_.resumeDownload();
        else
            updater_.pauseDownload();
    });
    connect(cancel_, &QPushButton::clicked, this, [this] {
        const auto result = QMessageBox::question(
            this, t("Download abbrechen"),
            t("Teildownload für später behalten? Nein entfernt die temporären Downloaddaten."),
            QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::No);
        if (result != QMessageBox::Cancel)
            updater_.cancelDownload(result == QMessageBox::Yes);
    });
    connect(skip_, &QPushButton::clicked, &u, &Updater::skipVersion);
    connect(install_, &QPushButton::clicked, this, [this] {
        if (QMessageBox::question(
                this, t("Installation bestätigen"),
                t("Prüfsumme und Buildnachweis sind gültig. Offene Änderungen müssen zuerst gespeichert "
                  "werden. Projekte, Bibliotheken und Einstellungen werden beibehalten. ThinkCad wird "
                  "beendet und anschließend neu gestartet. Fortfahren?"),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes)
            emit updater_.installRequested();
    });
    connect(&u, &Updater::changed, this, &UpdateDialog::refresh);
    connect(&u, &Updater::transferProgress, this,
            [this](qint64 bytes, qint64 total, double speed, qint64 remaining) {
                received_ = bytes;
                total_ = total;
                progress_->setRange(0, 1000);
                progress_->setValue(total ? int(bytes * 1000 / total) : 0);
                metrics_->setText(
                    QString("%1 / %2 MiB · %3 % · %4 MiB/s · %5")
                        .arg(bytes / 1048576., 0, 'f', 1)
                        .arg(total / 1048576., 0, 'f', 1)
                        .arg(total ? bytes * 100 / total : 0)
                        .arg(speed / 1048576., 0, 'f', 2)
                        .arg(remaining >= 0 ? QString::number(remaining) + " s" : t("Noch unbekannt")));
            });
    refresh();
}
void UpdateDialog::refresh() {
    release_->setText(t("Aktuelle Version") + ": " THINKERCAD_VERSION + "\n" + t("Verfügbare Version") +
                      ": " + updater_.version() + "\n" + t("Release-Datum") + ": " +
                      QLocale().toString(QDateTime::fromString(updater_.date(), Qt::ISODate).toLocalTime(),
                                         QLocale::ShortFormat) +
                      "\n" + t("Downloadgröße") + ": " +
                      (updater_.size() ? QString::number(updater_.size() / 1048576., 'f', 1) + " MiB"
                                       : t("Noch unbekannt")) +
                      "\n" +
                      (QSettings().value("updates/installed/seconds").toInt() > 0
                           ? t("Grobe Zeitschätzung anhand des letzten Updates:") + " " +
                                 QSettings().value("updates/installed/seconds").toString() + " s"
                           : t("Installationsdauer: abhängig von Datenträger, Sicherung und Dateisperren.")));
    status_->setText(updater_.status() + "\n" + updater_.details());
    security_->setText("SHA-256: " +
                       (updater_.hashValid()    ? t("Prüfsumme gültig")
                        : updater_.hashFailed() ? t("Ungültige Update-Prüfsumme.")
                                                : t("Noch nicht geprüft")) +
                       "\n" + t("Buildnachweis") + ": " +
                       (updater_.attestationValid()    ? t("Gültig: Repository, Workflow, Branch und Commit")
                        : updater_.attestationFailed() ? t("Buildnachweis ungültig. Keine Installation.")
                                                       : t("Noch nicht geprüft")));
    if (notes_->property("version") != updater_.version()) {
        notes_->setMarkdown(updater_.notes());
        notes_->setProperty("version", updater_.version());
    }
    using S = UpdateDownload::State;
    const auto s = updater_.downloadState();
    const bool active = s == S::Downloading || s == S::Retrying;
    download_->setEnabled(updater_.available() && !updater_.busy() && !updater_.ready() &&
                          !updater_.packageManaged());
    pause_->setText(s == S::Paused ? t("Fortsetzen") : t("Pausieren"));
    pause_->setEnabled(active || s == S::Paused);
    cancel_->setEnabled(updater_.busy() || active || s == S::Paused);
    if ((s == S::Paused || s == S::Retrying) && total_ > 0)
        metrics_->setText(QString("%1 / %2 MiB · 0 MiB/s")
                              .arg(received_ / 1048576., 0, 'f', 1)
                              .arg(total_ / 1048576., 0, 'f', 1));
    install_->setEnabled(updater_.ready() && !updater_.busy());
    skip_->setEnabled(updater_.available() && !updater_.busy());
    if (updater_.busy() && !active && s != S::Paused) {
        progress_->setRange(0, 0);
    } else if (updater_.ready()) {
        progress_->setRange(0, 100);
        progress_->setValue(100);
    }
    if (!updater_.busy() && !active && s != S::Paused) {
        progress_->setRange(0, 100);
        progress_->setValue(updater_.ready() ? 100 : 0);
    }
    if (updater_.packageManaged())
        metrics_->setText(t("Diese Installation wird über die Paketverwaltung aktualisiert. Gentoo: Overlay "
                            "aktualisieren und emerge --ask --oneshot media-gfx/thinkcad."));
}
