#include "mainwindow.hpp"
#include "feature135examples.hpp"
#include "projectshare.hpp"
#include "sharedialog.hpp"
#include "soundoutput.hpp"
#include "diagnosticspanel.hpp"
#include <QSoundEffect>
#include <QFile>
#include <QDialog>
#include <QPushButton>
#include <QLabel>
#include <QTimer>
#include <QAction>
#include "language.hpp"
#include <QtTest>
#include <QPlainTextEdit>
#include <QSettings>
#include <QTemporaryDir>
using namespace thinkercad;
class Feature135Tests : public QObject {
    Q_OBJECT
    QTemporaryDir settings_;
  private slots:
    void initTestCase() {
        QCoreApplication::setOrganizationName("ThinkCad135Tests");
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settings_.path());
        QApplication::setQuitOnLastWindowClosed(false);
        Language::set("en");
    }
    void init() {
        QSettings().clear();
        QSettings().setValue("soundEnabled", false);
        QSettings().setValue("updates/automatic", false);
    }
    void importPreviewRequiresConfirmationAndInvalidatesEdits() {
        auto project = ExampleCatalog::create("share_roundtrip");
        const auto encoded = ProjectShare::encode(project);
        int stage = 0;
        bool previewVisible = false, invalidated = false;
        QTimer poll;
        poll.setInterval(20);
        connect(&poll, &QTimer::timeout, this, [&] {
            auto *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
            if (!dialog)
                return;
            auto *code = dialog->findChild<QPlainTextEdit *>("importShareCode");
            auto *apply = dialog->findChild<QPushButton *>("confirmShareImport");
            auto *preview = dialog->findChild<QPushButton *>("previewShareImport");
            if (!code || !apply || !preview)
                return;
            if (stage == 0) {
                invalidated = !apply->isEnabled();
                code->setPlainText(encoded);
                preview->click();
                stage = 1;
            } else if (stage == 1 && apply->isEnabled()) {
                previewVisible = dialog->findChild<QLabel *>("shareImportPreview")
                                     ->text()
                                     .contains("Libraries are not installed automatically");
                code->setPlainText(encoded + " ");
                invalidated &= !apply->isEnabled();
                code->setPlainText(encoded);
                preview->click();
                stage = 2;
            } else if (stage == 2 && apply->isEnabled()) {
                stage = 3;
                apply->click();
            }
        });
        QTimer deadline;
        deadline.setSingleShot(true);
        connect(&deadline, &QTimer::timeout, this, [] {
            if (auto *d = qobject_cast<QDialog *>(QApplication::activeModalWidget()))
                d->reject();
        });
        deadline.start(10000);
        poll.start();
        const auto result = showShareImport(nullptr);
        poll.stop();
        deadline.stop();
        QVERIFY(result);
        QCOMPARE(stage, 3);
        QVERIFY(previewVisible);
        QVERIFY(invalidated);
        QCOMPARE(ProjectIO::deserialize(result->project).sketch.files, project.sketch.files);
    }
    void cancelImportPreservesCurrentProject() {
        MainWindow window;
        QVERIFY(window.loadExample("relay_button"));
        const auto parts = window.circuit().parts().size(), wires = window.circuit().wires().size();
        QTimer::singleShot(0, [] {
            if (auto *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget()))
                dialog->reject();
        });
        auto *importAction = window.findChild<QAction *>("importShareCode");
        QVERIFY(importAction);
        importAction->trigger();
        QCOMPARE(window.circuit().parts().size(), parts);
        QCOMPARE(window.circuit().wires().size(), wires);
        QVERIFY(!window.simulationActive());
    }
    void relayClickIsLocalAndGlobalMuteAvoidsAudioDevice() {
        QSettings().setValue("soundEnabled", false);
        QSettings().setValue("relayClickEnabled", false);
        SoundOutput sound;
        QVERIFY(sound.findChildren<QSoundEffect *>().empty());
        QFile click(":/audio/relay-click.wav");
        QVERIFY(click.open(QIODevice::ReadOnly));
        const auto wav = click.readAll();
        QVERIFY(wav.startsWith("RIFF") && wav.mid(8, 4) == "WAVE" && wav.size() > 44 && wav.size() < 4096);
        for (int i = 0; i < 100; ++i)
            sound.relayStates({{"relay", bool(i % 2)}});
        QVERIFY(sound.findChildren<QSoundEffect *>().empty());
        sound.stop();
    }
    void assistantUsesActualModelAndExplainsLimits() {
        DiagnosticsPanel panel;
        panel.show();
        auto *explanation = panel.findChild<QLabel *>("duckAssistantExplanation");
        QVERIFY(explanation);
        panel.offerHint(DiagnosticsPanel::Context::PartAdded, "relay", "relay-5v");
        QVERIFY(explanation->text().contains("COM, NO and NC"));
        panel.offerHint(DiagnosticsPanel::Context::Wired, "display", "ili9341");
        QVERIFY(explanation->text().contains("CS, DC, RESET"));
        panel.offerHint(DiagnosticsPanel::Context::PartAdded, "sensor", "bme680");
        QVERIFY(explanation->text().contains("chemical gas analysis is not modeled"));
        panel.offerHint(DiagnosticsPanel::Context::ShareUnencrypted);
        QVERIFY(explanation->text().contains("not confidential"));
    }
    void examplesRoundTrip() {
        for (const auto &entry : feature135Examples()) {
            QVERIFY(!entry.goal.isEmpty() && !entry.expected.isEmpty() && !entry.task.isEmpty());
            auto project = ExampleCatalog::create(entry.id);
            project.circuit.validate();
            const auto copy = ProjectIO::deserialize(ProjectIO::serialize(project));
            QCOMPARE(copy.circuit.parts().size(), project.circuit.parts().size());
            QCOMPARE(copy.circuit.wires().size(), project.circuit.wires().size());
            const auto share = ProjectShare::decode(ProjectShare::encode(project));
            QCOMPARE(ProjectIO::deserialize(share.project).sketch.files, project.sketch.files);
        }
    }
    void examplesRun_data() {
        QTest::addColumn<QString>("id");
        for (const auto &entry : feature135Examples())
            QTest::newRow(qPrintable(entry.id)) << entry.id;
    }
    void examplesRun() {
        QFETCH(QString, id);
        MainWindow window;
        window.show();
        QVERIFY(window.loadExample(id));
        if (id == "relay_button")
            for (const auto &p : window.circuit().parts())
                if (p.kind == PartKind::Button)
                    window.canvas()->setButtonPressed(p.id, true);
        window.startSimulation();
        auto *log = window.findChild<QPlainTextEdit *>("consoleOutput");
        auto *serial = window.findChild<QPlainTextEdit *>("serialMonitor");
        auto ready = [&] {
            if (!window.simulationActive())
                return false;
            const auto &state = window.circuitState();
            if (id.startsWith("tft_"))
                return std::any_of(
                    state.colorDisplays.begin(), state.colorDisplays.end(), [](const auto &item) {
                        return item.second &&
                               std::any_of(item.second->pixels.begin(), item.second->pixels.end(),
                                           [](auto color) { return (color & 0xffffff) != 0; });
                    });
            if (id == "bme680_monitor")
                return serial->toPlainText().contains("Simulated gas resistance ohm=");
            if (id == "mhz19_monitor")
                return serial->toPlainText().contains("CO2=812");
            if (id == "relay_button" || id == "share_roundtrip")
                return std::any_of(state.ledCurrent.begin(), state.ledCurrent.end(),
                                   [](const auto &item) { return item.second > .001; });
            for (const auto &p : window.circuit().parts())
                if (p.kind == PartKind::Module &&
                    (p.model == "stepper-4" || p.model == "stepper-5" || p.model == "stepper-6")) {
                    auto value = state.partValues.find(p.id);
                    if (value != state.partValues.end() && std::abs(value->second) > .1)
                        return true;
                }
            return false;
        };
        QTRY_VERIFY2_WITH_TIMEOUT(ready(), qPrintable(log->toPlainText() + "\n" + serial->toPlainText()),
                                  45000);
        QVERIFY(window.circuitState().errors.empty());
        if (id == "stepper_accel")
            QVERIFY(std::any_of(
                window.circuitState().partValues.begin(), window.circuitState().partValues.end(),
                [](const auto &item) { return item.first.ends_with("/target") && item.second == 20; }));
        window.stopSimulation();
    }
};
QTEST_MAIN(Feature135Tests)
#include "feature135_tests.moc"
