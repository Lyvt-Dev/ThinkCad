#include "updatedownload.hpp"
#include "mainwindow.hpp"
#include <QFileDialog>
#include <QPlainTextEdit>
#include "updater.hpp"
#include "updatedialog.hpp"
#include "appimageupdate.hpp"
#include "language.hpp"
#include "../packaging/windows/update_transaction.hpp"
#include <QtTest>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QPushButton>
#include <QLabel>
class UpdateTests : public QObject {
    Q_OBJECT
    QTcpServer server_;
    QByteArray body_ = QByteArray(4096, 'x');
    QList<QByteArray> requests_;
    QString mode_;
    QTemporaryDir settings_;
    QUrl url() {
        return QUrl(QString("http://127.0.0.1:%1/update").arg(server_.serverPort()));
    }
    QByteArray hash() {
        return QCryptographicHash::hash(body_, QCryptographicHash::Sha256);
    }
  private slots:
    void initTestCase() {
        QCoreApplication::setOrganizationName("ThinkCadUpdaterTests");
        QApplication::setAttribute(Qt::AA_DontUseNativeDialogs, true);
        qputenv("XDG_CACHE_HOME", settings_.filePath("cache").toUtf8());
        qputenv("XDG_DATA_HOME", settings_.filePath("data").toUtf8());
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settings_.path());
        QVERIFY(server_.listen(QHostAddress::LocalHost));
        connect(&server_, &QTcpServer::newConnection, this, [this] {
            while (server_.hasPendingConnections()) {
                auto *socket = server_.nextPendingConnection();
                auto request = std::make_shared<QByteArray>();
                connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
                connect(socket, &QTcpSocket::readyRead, socket, [this, socket, request] {
                    request->append(socket->readAll());
                    if (!request->contains("\r\n\r\n") || socket->property("sent").toBool())
                        return;
                    socket->setProperty("sent", true);
                    requests_.append(*request);
                    if (mode_ == "always-retry" || (mode_ == "retry" && requests_.size() == 1)) {
                        socket->write("HTTP/1.1 503 Busy\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
                        socket->disconnectFromHost();
                        return;
                    }
                    const auto match = QRegularExpression("Range: bytes=([0-9]+)-",
                                                          QRegularExpression::CaseInsensitiveOption)
                                           .match(QString::fromLatin1(*request));
                    int offset = match.hasMatch() ? match.captured(1).toInt() : 0;
                    bool range = offset > 0 && mode_ != "ignore";
                    if (!range)
                        offset = 0;
                    QByteArray headers = range ? "HTTP/1.1 206 Partial Content\r\n" : "HTTP/1.1 200 OK\r\n";
                    if (range)
                        headers += "Content-Range: bytes " +
                                   QByteArray::number(offset + (mode_ == "bad-range" ? 1 : 0)) +
                                   "-4095/4096\r\n";
                    headers +=
                        "ETag: \"fixture\"\r\nContent-Length: " + QByteArray::number(body_.size() - offset) +
                        "\r\nConnection: close\r\n\r\n";
                    socket->write(headers);
                    auto position = std::make_shared<int>(offset);
                    auto *timer = new QTimer(socket);
                    timer->setInterval(2);
                    connect(timer, &QTimer::timeout, socket, [this, socket, timer, position] {
                        if (socket->state() != QAbstractSocket::ConnectedState) {
                            timer->stop();
                            return;
                        }
                        socket->write(body_.mid(*position, 128));
                        *position += 128;
                        if (*position >= body_.size()) {
                            timer->stop();
                            socket->disconnectFromHost();
                        }
                    });
                    timer->start();
                });
            }
        });
    }
    void init() {
        requests_.clear();
        mode_.clear();
        QSettings().clear();
        Language::set("en");
    }
    void completeAndHashFailure() {
        QTemporaryDir dir;
        UpdateDownload d(nullptr, true);
        QSignalSpy ready(&d, &UpdateDownload::verified);
        d.start(url(), dir.filePath("setup"), body_.size(), hash());
        QTRY_COMPARE_WITH_TIMEOUT(ready.count(), 1, 5000);
        QCOMPARE(d.state(), UpdateDownload::State::Ready);
        UpdateDownload bad(nullptr, true);
        QSignalSpy fail(&bad, &UpdateDownload::failed);
        bad.start(url(), dir.filePath("bad"), body_.size(), QByteArray(32, 'a'));
        QTRY_COMPARE_WITH_TIMEOUT(fail.count(), 1, 5000);
        QVERIFY(!QFileInfo::exists(dir.filePath("bad.part")));
    }
    void restartResume_data() {
        QTest::addColumn<QString>("mode");
        QTest::newRow("range") << QString();
        QTest::newRow("ignored-range") << QString("ignore");
    }
    void restartResume() {
        QFETCH(QString, mode);
        mode_ = mode;
        QTemporaryDir dir;
        const auto path = dir.filePath("setup");
        {
            UpdateDownload d(nullptr, true);
            connect(&d, &UpdateDownload::progress, &d, [&](qint64 bytes, qint64, double, qint64) {
                if (bytes >= 512)
                    d.pause();
            });
            d.start(url(), path, body_.size(), hash());
            QTRY_COMPARE(d.state(), UpdateDownload::State::Paused);
            QVERIFY(QFileInfo(path + ".part").size() < body_.size());
            UpdateDownload locked(nullptr, true);
            locked.start(url(), path, body_.size(), hash());
            QCOMPARE(locked.state(), UpdateDownload::State::Error);
            d.cancel(true);
            QCOMPARE(d.state(), UpdateDownload::State::Cancelled);
        }
        UpdateDownload resumed(nullptr, true);
        QSignalSpy ready(&resumed, &UpdateDownload::verified);
        resumed.start(url(), path, body_.size(), hash());
        QTRY_COMPARE_WITH_TIMEOUT(ready.count(), 1, 5000);
        QVERIFY(requests_.last().toLower().contains("range: bytes="));
        QFile f(path);
        QVERIFY(f.open(QIODevice::ReadOnly));
        QCOMPARE(f.readAll(), body_);
    }
    void invalidRangeAndExplicitDiscard() {
        QTemporaryDir dir;
        const auto path = dir.filePath("setup");
        UpdateDownload d(nullptr, true);
        connect(&d, &UpdateDownload::progress, &d, [&](qint64 bytes, qint64, double, qint64) {
            if (bytes >= 512 && mode_.isEmpty())
                d.pause();
        });
        d.start(url(), path, body_.size(), hash());
        QTRY_COMPARE(d.state(), UpdateDownload::State::Paused);
        mode_ = "bad-range";
        QSignalSpy failed(&d, &UpdateDownload::failed);
        d.resume();
        QTRY_COMPARE_WITH_TIMEOUT(failed.count(), 1, 5000);
        QVERIFY(!QFileInfo::exists(path + ".part"));
        d.cancel(false);
        QVERIFY(!QFileInfo::exists(path + ".json"));
    }
    void boundedRetryAndProductionTransport() {
        mode_ = "retry";
        QTemporaryDir dir;
        UpdateDownload d(nullptr, true);
        QSignalSpy ready(&d, &UpdateDownload::verified);
        d.start(url(), dir.filePath("setup"), body_.size(), hash());
        QTRY_COMPARE_WITH_TIMEOUT(ready.count(), 1, 5000);
        QCOMPARE(requests_.size(), 2);
        UpdateDownload production;
        production.start(url(), dir.filePath("forbidden"), body_.size(), hash());
        QCOMPARE(production.state(), UpdateDownload::State::Error);
    }
    void retriesStopAfterThree() {
        mode_ = "always-retry";
        QTemporaryDir dir;
        UpdateDownload d(nullptr, true);
        QSignalSpy fail(&d, &UpdateDownload::failed);
        d.start(url(), dir.filePath("setup"), body_.size(), hash());
        QTRY_COMPARE_WITH_TIMEOUT(fail.count(), 1, 12000);
        QCOMPARE(requests_.size(), 4);
        QVERIFY(d.state() == UpdateDownload::State::Error);
    }
    void transactionRollbackAndCrashRecovery() {
        namespace fs = std::filesystem;
        QTemporaryDir dir;
        const auto bytes = dir.path().toUtf8().toStdString();
        const auto root = fs::path(std::u8string(bytes.begin(), bytes.end()));
        fs::create_directory(root / "ThinkCad");
        std::ofstream(root / "ThinkCad" / "app") << "old";
        std::ofstream(root / "ThinkCad" / "local-library") << "user";
        auto t = thinkcad_update::Transaction::create(root / "ThinkCad", root / "journal", "abc-123");
        t.prepare([](auto, auto) {});
        std::ofstream(t.target / "app") << "broken";
        auto recovered = thinkcad_update::Transaction::load(root / "journal");
        recovered.rollback();
        std::ifstream f(root / "ThinkCad" / "app");
        std::string text;
        f >> text;
        f.close();
        QCOMPARE(text, std::string("old"));
        QVERIFY(fs::exists(root / "ThinkCad" / "local-library"));
        QVERIFY(fs::exists(t.failed / "app"));
        auto cancelled =
            thinkcad_update::Transaction::create(root / "ThinkCad", root / "journal2", "abc-456");
        try {
            cancelled.prepare([](auto, auto) { throw std::runtime_error("cancel"); });
            QFAIL("copy should cancel");
        } catch (const std::runtime_error &) {
        }
        cancelled.rollback();
        QVERIFY(fs::exists(root / "ThinkCad" / "local-library"));
        auto committed =
            thinkcad_update::Transaction::create(root / "ThinkCad", root / "journal3", "abc-789");
        committed.prepare([](auto, auto) {});
        std::ofstream(committed.target / "app") << "new";
        committed.commit();
        auto done = thinkcad_update::Transaction::load(root / "journal3");
        done.rollback();
        QVERIFY(fs::exists(done.backup / "app"));
        std::ifstream newer(done.target / "app");
        newer >> text;
        QCOMPARE(text, std::string("new"));
    }
    void cancellingProjectSaveBlocksInstallation() {
        MainWindow window;
        window.show();
        QTest::qWait(20);
        auto *editor = window.findChild<QPlainTextEdit *>("codeEditor");
        if (!editor)
            editor = window.findChild<QPlainTextEdit *>();
        QVERIFY(editor);
        editor->insertPlainText("\n// unsaved update test\n");
        auto *updater = window.findChild<Updater *>();
        QVERIFY(updater);
        bool cancelled = false;
        QTimer::singleShot(50, &window, [&] {
            if (auto *dialog = qobject_cast<QFileDialog *>(QApplication::activeModalWidget())) {
                cancelled = true;
                dialog->reject();
            }
        });
        emit updater->installRequested();
        QVERIFY(cancelled);
        QVERIFY(updater->status().isEmpty());
        QVERIFY(!updater->ready());
    }
    void dialogAndPreferencesAreLocal() {
        Updater updater;
        UpdateDialog dialog(updater);
        QVERIFY(!dialog.findChild<QPushButton *>("updateInstall")->isEnabled());
        QVERIFY(!dialog.findChild<QPushButton *>("updatePause")->isEnabled());
        dialog.show();
        QTest::qWait(20);
        if (!qEnvironmentVariable("THINKCAD_REVIEW_DIR").isEmpty()) {
            QDir().mkpath(qEnvironmentVariable("THINKCAD_REVIEW_DIR"));
            dialog.grab().save(qEnvironmentVariable("THINKCAD_REVIEW_DIR") + "/updater.png");
        }
    }
};
QTEST_MAIN(UpdateTests)
#include "updater_tests.moc"
