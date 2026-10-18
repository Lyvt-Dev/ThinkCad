#include "mainwindow.hpp"
#include "circuitcanvas.hpp"
#include "blockeditor/blockeditor.hpp"
#include "blockeditor/blockpersistence.hpp"
#include "thinkercad/visual/expressionparser.hpp"
#include "thinkercad/measurement/acquisition.hpp"
#include "projectexport.hpp"
#include "starterprojects.hpp"
#include "collaborationproject.hpp"
#include "paths.hpp"
#include "language.hpp"
#include <QtTest>
#include <QApplication>
#include <QSettings>
#include <QTemporaryDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QPlainTextEdit>
#include <QTabWidget>
#include <QTreeWidget>
#include "wiringgeometry.hpp"
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>
#include <QDockWidget>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QGraphicsView>
#include <QDir>
#include <QSvgRenderer>
#include <QProcess>
using namespace thinkercad;
using namespace thinkercad::visual;
class Feature129Tests : public QObject {
    Q_OBJECT
    QTemporaryDir temp_;
    static Expression expression(const char *s) {
        return ExpressionParser::parse(s);
    }
    static Block call(const char *id, const char *name, std::vector<Expression> args) {
        Block b;
        b.id = id;
        b.name = name;
        b.arguments = std::move(args);
        return b;
    }
    static SketchProject sketch(const QString &source) {
        SketchProject p;
        p.order = {"main.ino"};
        p.files["main.ino"] = source;
        return p;
    }
  private slots:
    void initTestCase() {
        QCoreApplication::setOrganizationName("ThinkCad129Tests");
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, temp_.path());
        QApplication::setQuitOnLastWindowClosed(false);
    }
    void init() {
        QSettings().clear();
        Language::set("en");
    }
    void cleanup() {
        QSettings().clear();
        Language::set("de");
    }
    void everyCommandGeneratesCompilableArduino() {
        BlockProgram p;
        int n = 0;
        for (const auto &spec : ArduinoCodeGenerator::commands()) {
            Block b;
            b.id = "command" + std::to_string(++n);
            b.name = spec.name;
            for (auto type : spec.arguments)
                b.arguments.push_back(type == ValueType::Text ? expression("\"test\"") : expression("1"));
            if (spec.name == "analogRead")
                b.arguments[0] = expression("14");
            if (spec.name == "analogWrite")
                b.arguments[0] = expression("5");
            if (spec.name == "pinMode")
                b.arguments[1] = expression("OUTPUT");
            p.loop.push_back(b);
        }
        Function fun;
        fun.id = "function";
        fun.name = "echoText";
        fun.result = ValueType::Text;
        fun.parameters = {{"message", ValueType::Text, {}}};
        Block ret;
        ret.id = "return";
        ret.kind = Block::Kind::Return;
        ret.arguments = {expression("message")};
        fun.body = {ret};
        p.functions = {fun};
        auto result = ArduinoCodeGenerator::generate(p, BoardProfile::get("arduino-uno"));
        QVERIFY2(result.valid(), result.issues.empty() ? "" : result.issues[0].code.c_str());
        const auto path = temp_.filePath("generated.cpp");
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(QByteArray::fromStdString(result.source));
        file.close();
        QProcess compiler;
        compiler.start(AppPaths::compiler(), {"-std=c++20", "-fsyntax-only", "-I" + AppPaths::sdk(), path});
        QVERIFY(compiler.waitForFinished(60000));
        QVERIFY2(compiler.exitCode() == 0, compiler.readAllStandardError().constData());
    }
    void metadataRoundTripsAndLegacyLoads() {
        SavedProject project;
        project.sketch = sketch("void setup(){} void loop(){}");
        BlockProgram p;
        p.setup = {call("start", "Serial.begin", {expression("115200")})};
        project.sketch.visual = {{"program", BlockPersistence::encode(p)},
                                 {"mode", 1},
                                 {"futureExtension", QJsonObject{{"keep", true}}}};
        project.boardSketches["board"] = project.sketch;
        const auto data = ProjectIO::serialize(project);
        QCOMPARE(QJsonDocument::fromJson(data).object()["version"].toInt(), 8);
        auto restored = ProjectIO::deserialize(data);
        QCOMPARE(restored.boardSketches["board"].visual, project.sketch.visual);
        QCOMPARE(
            CollaborationProject::decode(CollaborationProject::encode(project)).boardSketches["board"].visual,
            project.sketch.visual);
        auto old = QJsonDocument::fromJson(data).object();
        old["version"] = 6;
        old.remove("visual");
        QCOMPARE(ProjectIO::deserialize(QJsonDocument(old).toJson()).sketch.files, project.sketch.files);
        auto invalid = BlockPersistence::encode(p);
        invalid["setup"] = QJsonArray{QJsonObject{{"kind", 500}}};
        QVERIFY_EXCEPTION_THROWN(BlockPersistence::decode(invalid), std::runtime_error);
    }
    void manualCppIsPreservedAndResetKeepsBackup() {
        BlockProgram p;
        p.loop = {call("delay", "delay", {expression("10")})};
        const auto source =
            QString::fromStdString(ArduinoCodeGenerator::generate(p, BoardProfile::get("esp8266")).source);
        auto project = sketch(source);
        project.visual = {{"program", BlockPersistence::encode(p)},
                          {"fingerprint", BlockPersistence::fingerprint(source)},
                          {"mode", 1}};
        BlockEditor editor;
        editor.setSketch(project, BoardProfile::get("esp8266"));
        QVERIFY(editor.findChild<QLabel *>("blockSyncStatus")->text().contains("synchron"));
        const auto manual = source + "// manual change\n";
        editor.sourceEdited(manual);
        QVERIFY(
            editor.findChild<QLabel *>("blockSyncStatus")
                ->text()
                .contains("not synchron", Qt::CaseInsensitive) ||
            editor.findChild<QLabel *>("blockSyncStatus")->text().contains("changed", Qt::CaseInsensitive));
        QSignalSpy applied(&editor, &BlockEditor::programApplied);
        QTimer::singleShot(0, [] {
            auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
            if (box)
                box->button(QMessageBox::Yes)->click();
        });
        editor.findChild<QPushButton *>("regenerateBlocks")->click();
        QCOMPARE(applied.size(), 1);
        QCOMPARE(applied[0][0].toJsonObject()["backup"].toString(), manual);
        QCOMPARE(applied[0][1].toString(), source);
    }
    void editorDoesNotResetManualChanges() {
        MainWindow window;
        auto *tabs = window.findChild<QTabWidget *>("codeTabs");
        auto *source = qobject_cast<QPlainTextEdit *>(tabs->currentWidget());
        QVERIFY(source);
        auto before = source->toPlainText();
        source->appendPlainText("// local edit");
        QVERIFY(source->toPlainText().startsWith(before));
        QVERIFY(source->toPlainText().contains("// local edit"));
    }
    void breadboardPreviewUsesSharedNetAndKeepsUndo() {
        CircuitDocument doc;
        auto board = doc.addPart(PartKind::Breadboard30, 0, 0);
        doc.connect({"board", "GPIO5"}, {board, "a1"});
        CircuitCanvas canvas(doc);
        canvas.previewNet({board, "e1"});
        int highlighted = 0;
        for (auto *item : canvas.scene()->items())
            if (item->data(3).toBool())
                ++highlighted;
        QCOMPARE(highlighted, 6);
        QVERIFY(canvas.scene()->selectedItems().isEmpty());
        canvas.clearNetPreview();
        canvas.mutate("connect", [&](auto &d) { d.connect({board, "a1"}, {board, "f1"}); });
        QVERIFY(canvas.topology().connected({board, "a1"}, {board, "j1"}));
        canvas.history()->undo();
        QVERIFY(!canvas.topology().connected({board, "a1"}, {board, "j1"}));
    }
    void editingParentKeepsBranchesAndPythonRemainsCppOnly() {
        Block condition;
        condition.id = "condition";
        condition.kind = Block::Kind::If;
        condition.arguments = {expression("true")};
        condition.body = {call("yes", "delay", {expression("1")})};
        condition.otherwise = {call("no", "delay", {expression("2")})};
        BlockProgram p;
        p.loop = {condition};
        auto source =
            QString::fromStdString(ArduinoCodeGenerator::generate(p, BoardProfile::get("esp8266")).source);
        auto project = sketch(source);
        project.visual = {{"program", BlockPersistence::encode(p)},
                          {"fingerprint", BlockPersistence::fingerprint(source)},
                          {"mode", 1}};
        BlockEditor editor;
        editor.setSketch(project, BoardProfile::get("esp8266"));
        editor.findChild<QTreeWidget *>("blockTree")
            ->setCurrentItem(editor.findChild<QTreeWidget *>("blockTree")->topLevelItem(1)->child(0));
        editor.findChild<QPlainTextEdit *>("blockArguments")->setPlainText("false");
        QSignalSpy applied(&editor, &BlockEditor::programApplied);
        editor.findChild<QPushButton *>("applyBlock")->click();
        QCOMPARE(applied.size(), 1);
        auto restored = BlockPersistence::decode(applied[0][0].toJsonObject()["program"].toObject());
        QCOMPARE(restored.loop[0].body[0].id, std::string("yes"));
        QCOMPARE(restored.loop[0].otherwise[0].id, std::string("no"));
        project.language = "micropython";
        project.order = {"main.py"};
        project.files = {{"main.py", "print(1)"}};
        editor.setSketch(project, BoardProfile::get("esp8266"));
        editor.setExecutionLocked(true);
        editor.setExecutionLocked(false);
        QVERIFY(!editor.findChild<QComboBox *>("codingMode")->isEnabled());
        QVERIFY(!editor.findChild<QPushButton *>("regenerateBlocks")->isEnabled());
    }
    void auxiliaryCppChangesBreakSynchronization() {
        BlockProgram p;
        p.loop = {call("delay", "delay", {expression("10")})};
        auto source =
            QString::fromStdString(ArduinoCodeGenerator::generate(p, BoardProfile::get("esp8266")).source);
        auto project = sketch(source);
        project.visual = {{"program", BlockPersistence::encode(p)},
                          {"fingerprint", BlockPersistence::fingerprint(source)},
                          {"mode", 1}};
        BlockEditor editor;
        editor.setSketch(project, BoardProfile::get("esp8266"));
        project.files["helper.h"] = "int value=1;";
        project.order << "helper.h";
        editor.sourcesEdited(project);
        QVERIFY(editor.findChild<QLabel *>("blockSyncStatus")->text().contains("Not synchronized"));
    }
    void visualStarterAndThreeViews() {
        MainWindow window;
        QVERIFY(window.loadExample("starter_blink"));
        window.resize(1400, 900);
        window.show();
        QTest::qWait(30);
        auto *mode = window.findChild<QComboBox *>("codingMode");
        auto *blocks = window.findChild<QGraphicsView *>("blockWorkspace");
        auto *tabs = window.findChild<QTabWidget *>("codeTabs");
        QCOMPARE(mode->currentIndex(), 1);
        QVERIFY(blocks->isVisible());
        QVERIFY(tabs->isVisible());
        QVERIFY(window.height() <= 1000);
        mode->setCurrentIndex(2);
        QVERIFY(!blocks->isVisible());
        QVERIFY(tabs->isVisible());
        mode->setCurrentIndex(0);
        QVERIFY(blocks->isVisible());
        QVERIFY(!tabs->isVisible());
        mode->setCurrentIndex(1);
        const auto review = qEnvironmentVariable("THINKCAD_REVIEW_DIR");
        if (!review.isEmpty()) {
            QDir().mkpath(review);
            QVERIFY(window.grab().save(review + "/visual-starter.png"));
        }
    }
    void geometricOverlapIsNotACrossing() {
        QVERIFY(overlappingSegments(QLineF(0, 0, 10, 0), QLineF(4, 0, 20, 0)));
        QVERIFY(!overlappingSegments(QLineF(0, 0, 10, 0), QLineF(5, -5, 5, 5)));
        QVERIFY(!overlappingSegments(QLineF(0, 0, 10, 0), QLineF(10, 0, 20, 0)));
    }
    void boundedAcquisitionAndCsv() {
        using namespace measurement;
        TraceStore store(3);
        store.append({0, {0., {}}});
        store.append({10, {1., 2.}});
        store.append({10, {2., 3.}});
        QCOMPARE(store.samples().size(), std::size_t(2));
        QVERIFY(crossed(store.samples().front(), store.samples().back(), 0, 1, true));
        store.append({20, {{}, 4.}});
        store.append({30, {5., 6.}});
        QCOMPARE(store.overwritten(), std::uint64_t(1));
        QVERIFY(!store.valueAt(0, 0));
        QVERIFY(!store.valueAt(25, 0));
        QCOMPARE(store.valueAt(15, 0).value(), 2.);
        QVERIFY_EXCEPTION_THROWN(store.append({5, {1.}}), std::invalid_argument);
        const auto output = measurement::csv(store.samples(), {"A_V", "B_V"});
        QVERIFY(output.find("2e-05,,4") != std::string::npos);
        CircuitState state;
        CircuitPart scope{"scope", PartKind::Oscilloscope};
        state.voltage[{"scope", "A"}] = 3.;
        auto point = acquire(scope, 1, state);
        QVERIFY(!point.channels[0]);
        state.voltage[{"scope", "GND"}] = 1.;
        QCOMPARE(acquire(scope, 1, state).channels[0].value(), 2.);
    }
    void imageBomAndZipExport() {
        SavedProject p;
        p.sketch = sketch("void setup(){} void loop(){}");
        p.circuit.addPart(PartKind::Resistor, 300, 20);
        p.circuit.addPart(PartKind::Resistor, 300, 200);
        auto bom = BomBuilder::csv(p.circuit);
        QVERIFY(bom.contains(",2,"));
        CircuitCanvas canvas(p.circuit);
        canvas.resize(800, 600);
        canvas.show();
        QTest::qWait(10);
        const auto png = temp_.filePath("circuit.png"), svg = temp_.filePath("circuit.svg");
        CircuitExporter::save(canvas, png);
        CircuitExporter::save(canvas, svg);
        QVERIFY(!QImage(png).isNull());
        QSvgRenderer renderer(svg);
        QVERIFY(renderer.isValid());
        auto files = ProjectExporter::files(p, {"/private/user/library"});
        QVERIFY(files.contains("project.thinkcad"));
        QVERIFY(files.contains("sketches/unassigned/main/main.ino"));
        QVERIFY(!files["libraries.json"].contains("/private/user"));
        QCOMPARE(ProjectIO::deserialize(files["project.thinkcad"]).sketch.files, p.sketch.files);
        const auto zip = temp_.filePath("project.zip");
        ProjectExporter::saveZip(zip, p, {});
        QProcess python;
        python.start(
            AppPaths::python(),
            {"-c",
             "import zipfile,sys,json; z=zipfile.ZipFile(sys.argv[1]); assert z.testzip() is None; assert "
             "json.loads(z.read('project.thinkcad'))['version']==8; assert 'README.md' in z.namelist()",
             zip});
        QVERIFY(python.waitForFinished(10000));
        QVERIFY2(python.exitCode() == 0, python.readAllStandardError().constData());
        QVERIFY_EXCEPTION_THROWN(ZipWriter::archive({{"../escape", "bad"}}), std::runtime_error);
        QVERIFY_EXCEPTION_THROWN(ZipWriter::archive({{"CON.txt", "bad"}}), std::runtime_error);
    }
    void startersRun_data() {
        QTest::addColumn<QString>("id");
        for (const auto &s : StarterProjects::entries()) {
            QVERIFY(!s.goal.isEmpty() && !s.expected.isEmpty());
            QTest::newRow(qPrintable(s.id)) << s.id;
        }
    }
    void startersRun() {
        QFETCH(QString, id);
        MainWindow window;
        QVERIFY(window.loadExample(id));
        if (id == "starter_button")
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
            if (id == "starter_blink" || id == "starter_button" || id == "starter_pwm" ||
                id == "starter_pot") {
                for (const auto &[part, current] : state.ledCurrent)
                    if (current > .0001)
                        return true;
                return false;
            }
            if (id == "starter_temperature") {
                for (const auto &part : window.circuit().parts())
                    if (part.kind == PartKind::Oled) {
                        const auto &frame = window.displayFrame(part.id);
                        for (auto byte : frame.pixels)
                            if (byte)
                                return true;
                    }
                return false;
            }
            if (id == "starter_dht")
                return serial->toPlainText().contains("Humidity: 50");
            if (id == "starter_ultrasonic")
                return serial->toPlainText().contains("Distance (cm): 100");
            if (id == "starter_uart")
                return log->toPlainText().contains("UART_OK");
            if (id == "starter_scanner")
                return serial->toPlainText().contains("Found 0x3C") &&
                       serial->toPlainText().contains("Found 0x76");
            if (id == "starter_pixels")
                return !state.pixels.empty() &&
                       std::any_of(state.pixels.begin(), state.pixels.end(), [](const auto &entry) {
                           return std::any_of(entry.second.begin(), entry.second.end(),
                                              [](auto color) { return color != 0; });
                       });
            if (id == "starter_servo" || id == "starter_motor") {
                for (const auto &p : window.circuit().parts())
                    if (p.kind == (id == "starter_servo" ? PartKind::ServoMotor : PartKind::DcMotor)) {
                        auto it = state.partValues.find(p.id);
                        if (it != state.partValues.end() && it->second > 1)
                            return true;
                    }
                return false;
            }
            return false;
        };
        QTRY_VERIFY2_WITH_TIMEOUT(ready(), qPrintable(log->toPlainText()), 20000);
        QVERIFY(window.circuitState().errors.empty());
        window.stopSimulation();
    }
};
QTEST_MAIN(Feature129Tests)
#include "feature129_tests.moc"
