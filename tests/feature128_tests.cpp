#include "mainwindow.hpp"
#include "startpage.hpp"
#include "pythontools.hpp"
#include "collaborationproject.hpp"
#include "paths.hpp"
#include "language.hpp"
#include <QtTest>
#include <QAction>
#include <cmath>
#include <QApplication>
#include <QSettings>
#include <QTemporaryDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QComboBox>
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QGroupBox>
#include <QLabel>
using namespace thinkercad;
class Feature128Tests final:public QObject {
    Q_OBJECT
    QTemporaryDir temporary_;
    SavedProject python(const QString& source,const std::string& model="esp32"){
        SavedProject project;project.circuit.part("board").model=model;project.sketch.language="micropython";project.sketch.order={"main.py"};project.sketch.files["main.py"]=source;project.boardSketches["board"]=project.sketch;return project;
    }
    QString save(const SavedProject& project,const QString& name="python.thinkcad"){
        const auto path=temporary_.filePath(name);ProjectIO::save(path,project);return path;
    }
    static bool write(const QString& path,const QByteArray& bytes){QFile file(path);return file.open(QIODevice::WriteOnly)&&file.write(bytes)==bytes.size();}
private slots:
    void initTestCase(){QCoreApplication::setOrganizationName("ThinkCad128Tests");QSettings::setDefaultFormat(QSettings::IniFormat);QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,temporary_.path());QApplication::setQuitOnLastWindowClosed(false);}
    void init(){QSettings().clear();Language::set("en");}
    void cleanup(){QSettings().clear();Language::set("de");}
    void projectLanguageAndLegacyMigration(){
        auto project=python("print('native MicroPython')\n");auto restored=ProjectIO::deserialize(ProjectIO::serialize(project));QCOMPARE(restored.sketch.language,QString("micropython"));QCOMPARE(restored.boardSketches["board"].language,QString("micropython"));QCOMPARE(restored.sketch.entryPoint(),QString("main.py"));
        auto shared=CollaborationProject::encode(project);QVERIFY(CollaborationProject::validate(shared).isEmpty());QCOMPARE(CollaborationProject::decode(shared).boardSketches["board"].language,QString("micropython"));
        auto invalid=QJsonDocument::fromJson(ProjectIO::serialize(project)).object();invalid["language"]="javascript";QVERIFY_EXCEPTION_THROWN(ProjectIO::deserialize(QJsonDocument(invalid).toJson()),std::runtime_error);
        SavedProject legacy;legacy.sketch.order={"old.ino"};legacy.sketch.files["old.ino"]="void setup(){} void loop(){}";auto object=QJsonDocument::fromJson(ProjectIO::serialize(legacy)).object();object["version"]=5;object.remove("language");QCOMPARE(ProjectIO::deserialize(QJsonDocument(object).toJson()).sketch.language,QString("arduino"));
    }
    void nativeMicroPythonGpioAndPause_data(){QTest::addColumn<bool>("emulation");QTest::newRow("simulation")<<false;QTest::newRow("emulation")<<true;}
    void nativeMicroPythonGpioAndPause(){
        QFETCH(bool,emulation);QSettings().setValue("compiler","missing-cxx-compiler");
        auto project=python("import sys\nassert sys.implementation.name == 'micropython'\nfrom machine import Pin\nfrom time import sleep_ms\nimport auxiliary\nassert auxiliary.answer == 42\nled=Pin(2,Pin.OUT)\nwhile True:\n    led.on()\n    sleep_ms(1000)\n    led.off()\n    sleep_ms(1000)\n");project.sketch.order<<"auxiliary.py";project.sketch.files["auxiliary.py"]="answer=42\n";project.boardSketches["board"]=project.sketch;project.mode=emulation?ExecutionMode::Emulation:ExecutionMode::Simulation;
        auto resistor=project.circuit.addPart(PartKind::Resistor,400,0),led=project.circuit.addPart(PartKind::Led,500,0);project.circuit.connect({"board","GPIO2"},{resistor,"1"});project.circuit.connect({resistor,"2"},{led,"A"});project.circuit.connect({led,"K"},{"board","GND"});
        MainWindow window;QVERIFY(window.openPath(save(project)));QVERIFY(!window.simulationActive());window.startSimulation();QTRY_VERIFY_WITH_TIMEOUT(window.circuitState().ledCurrent.at(led)>.005,5000);QVERIFY(window.simulationActive());
        auto* pause=window.findChild<QAction*>("pauseSimulation");pause->setChecked(true);QTest::qWait(100);const auto time=window.simulationTime();QTest::qWait(100);QCOMPARE(window.simulationTime(),time);pause->setChecked(false);QTRY_VERIFY(window.simulationTime()>time);window.stopSimulation();
    }
    void i2cAndSpiUsePhysicalWiring(){
        auto project=python("from machine import Pin,I2C,SPI\nfrom time import sleep_ms\ni2c=I2C(0,sda=Pin(21),scl=Pin(22))\nassert i2c.readfrom_mem(0x76,0xd0,1)==b'\\x60'\ncs=Pin(5,Pin.OUT,value=1)\nspi=SPI(1,sck=Pin(18),miso=Pin(19),mosi=Pin(23))\ncs.off()\nrx=bytearray(2)\nspi.write_readinto(b'\\xd0\\x00',rx)\ncs.on()\nassert rx[1]==0x60\nled=Pin(2,Pin.OUT,value=1)\nwhile True: sleep_ms(20)\n");
        auto i2c=project.circuit.addPart(PartKind::Module,0,0),spi=project.circuit.addPart(PartKind::Module,0,0);project.circuit.part(i2c).model="bme280";project.circuit.part(spi).model="bme280-spi";
        for(const auto& id:{i2c,spi}){project.circuit.connect({id,"VCC"},{"board","3V3"});project.circuit.connect({id,"GND"},{"board","GND"});}
        project.circuit.connect({i2c,"SDA"},{"board","GPIO21"});project.circuit.connect({i2c,"SCL"},{"board","GPIO22"});
        for(const auto& pair:std::vector<std::pair<std::string,std::string>>{{"SCK","GPIO18"},{"MISO","GPIO19"},{"MOSI","GPIO23"},{"CS","GPIO5"}})project.circuit.connect({spi,pair.first},{"board",pair.second});
        MainWindow window;QVERIFY(window.openPath(save(project)));window.startSimulation();QTRY_VERIFY_WITH_TIMEOUT(window.circuitState().level({"board","GPIO2"})==Level::High,5000);QVERIFY(window.simulationActive());window.stopSimulation();
        project.circuit.clearWires();QVERIFY(window.openPath(save(project)));window.startSimulation();QTRY_VERIFY_WITH_TIMEOUT(!window.simulationActive(),5000);QVERIFY(window.circuitState().level({"board","GPIO2"})!=Level::High);
    }
    void adcAndPwm(){
        auto project=python("from machine import Pin,ADC,PWM\nfrom time import sleep_ms\nadc=ADC(Pin(36))\nassert 32000<adc.read_u16()<33500\npwm=PWM(Pin(2),freq=1000,duty_u16=32768)\nwhile True: sleep_ms(20)\n");
        auto source=project.circuit.addPart(PartKind::AnalogSource,0,0);project.circuit.part(source).value=1.65;project.circuit.connect({source,"GND"},{"board","GND"});project.circuit.connect({source,"OUT"},{"board","GPIO36"});
        const auto resistor=project.circuit.addPart(PartKind::Resistor,0,0),capacitor=project.circuit.addPart(PartKind::Capacitor,0,0);project.circuit.part(resistor).value=1000;project.circuit.part(capacitor).value=10;project.circuit.connect({"board","GPIO2"},{resistor,"1"});project.circuit.connect({resistor,"2"},{capacitor,"1"});project.circuit.connect({capacitor,"2"},{"board","GND"});
        MainWindow window;QVERIFY(window.openPath(save(project)));window.startSimulation();QTRY_VERIFY2_WITH_TIMEOUT(window.circuitState().voltage.at({capacitor,"1"}).value_or(0)>1.5,qPrintable(window.findChild<QPlainTextEdit*>("consoleOutput")->toPlainText()),5000);QVERIFY(std::abs(*window.circuitState().voltage.at({capacitor,"1"})-1.65)<.2);QVERIFY(window.simulationActive());window.stopSimulation();
    }
    void cppAndMicroPythonBoardsExchangeUart(){
        SavedProject project;project.mode=ExecutionMode::Emulation;project.circuit.part("board").model="esp32";project.sketch.order={"sender.ino"};project.sketch.files["sender.ino"]="void setup(){Serial1.begin(9600,SERIAL_8N1,16,17);}void loop(){Serial1.write(42);delay(50);}";project.boardSketches["board"]=project.sketch;
        auto receiver=project.circuit.addPart(PartKind::Esp8266,500,0);project.circuit.part(receiver).model="esp32";project.boardSketches[QString::fromStdString(receiver)]=python("from machine import Pin,UART\nfrom time import sleep_ms\nuart=UART(1,9600,rx=Pin(16),tx=Pin(17),timeout=1000)\nled=Pin(2,Pin.OUT)\nwhile True:\n    data=uart.read(1)\n    if data==b'*':led.on()\n    sleep_ms(10)\n").sketch;
        project.circuit.connect({"board","GND"},{receiver,"GND"});project.circuit.connect({"board","GPIO17"},{receiver,"GPIO16"});MainWindow window;QVERIFY(window.openPath(save(project)));window.startSimulation();QTRY_VERIFY_WITH_TIMEOUT(window.circuitState().level({receiver,"GPIO2"})==Level::High,15000);QVERIFY(window.simulationActive());window.stopSimulation();
    }
    void localPythonRunsUnicodeImportsAndStops(){
        QVERIFY2(!AppPaths::python().isEmpty(),"Python 3 is required for local-script verification");const auto script=temporary_.filePath("local.py");QVERIFY(write(temporary_.filePath("helper.py"),"answer=42\n"));QVERIFY(write(script,"import helper,sys,time\nassert helper.answer==42\nfor b in 'Grüße'.encode('utf-8'):\n sys.stdout.buffer.write(bytes([b]));sys.stdout.buffer.flush();time.sleep(.002)\n"));
        PythonTools dialog;QVERIFY(dialog.openFile(script));dialog.findChild<QPushButton*>("pythonRun")->click();QTRY_VERIFY_WITH_TIMEOUT(!dialog.running(),10000);QVERIFY(dialog.findChild<QPlainTextEdit*>("pythonOutput")->toPlainText().contains(QString::fromUtf8("Grüße")));
        dialog.findChild<QPlainTextEdit*>("pythonSource")->setPlainText("while True: pass\n");dialog.findChild<QPushButton*>("pythonRun")->click();QTRY_VERIFY(dialog.running());dialog.findChild<QPushButton*>("pythonStop")->click();QTRY_VERIFY_WITH_TIMEOUT(!dialog.running(),2000);
    }
    void newLocalesHaveCompleteCatalogsAndSwitchLive(){
        QFile file(":/i18n/catalog-additional.json");QVERIFY(file.open(QIODevice::ReadOnly));const auto catalog=QJsonDocument::fromJson(file.readAll()).object();for(const auto& source:Language::sourceTexts())QVERIFY2(catalog.contains(source),qPrintable(source));
        for(auto it=catalog.begin();it!=catalog.end();++it){const auto row=it.value().toArray();QCOMPARE(row.size(),2);for(const auto& value:row){const auto text=value.toString();QVERIFY(!text.isEmpty());QVERIFY(!text.contains("<unk>"));QVERIFY(!text.contains(QString::fromUtf8("⁇")));for(int n=1;n<=9;++n){const auto token="%"+QString::number(n);QCOMPARE(text.count(token),it.key().count(token));}}}
        MainWindow window;auto* selector=window.findChild<QComboBox*>("languageSelector");QCOMPARE(selector->count(),8);selector->setCurrentIndex(selector->findData("it"));QCOMPARE(window.findChild<QPushButton*>("homeNavigation")->text(),QString("I miei progetti"));QCOMPARE(Language::text("Einstellungen"),QString("Impostazioni"));selector->setCurrentIndex(selector->findData("pt"));QCOMPARE(window.findChild<QPushButton*>("homeNavigation")->text(),QString("Meus projetos"));QCOMPARE(Language::text("Einstellungen"),QString::fromUtf8("Configurações"));selector->setCurrentIndex(selector->findData("de"));QCOMPARE(window.findChild<QPushButton*>("homeNavigation")->text(),QString("Meine Projekte"));
    }
    void sidebarsFilterAndPreserveCollapsedGroups(){
        MainWindow window;auto* categories=window.findChild<QComboBox*>("componentCategoryFilter");auto* search=window.findChild<QLineEdit*>("componentSearch");QVERIFY(categories&&search);QVERIFY(categories->count()>5);QGroupBox* group=nullptr;for(auto* candidate:window.findChildren<QGroupBox*>())if(candidate->property("category").toString()=="Anzeigen")group=candidate;QVERIFY(group);group->setChecked(false);QVERIFY(!QSettings().value("componentGroups/Anzeigen").toBool());search->setText("LED");QVERIFY(group->isChecked());QVERIFY(!group->isHidden());search->clear();QVERIFY(!group->isChecked());group->setChecked(true);
        auto* home=window.findChild<WelcomePage*>();QVERIFY(home->findChild<QPushButton*>("navigationPython"));QVERIFY(home->findChild<QPushButton*>("navigationChangelog"));QVERIFY(home->findChild<QLineEdit*>("recentProjectSearch"));
        const auto a=save(python("print(1)\n"),"A.thinkcad"),z=save(python("print(2)\n"),"Z.thinkcad");WelcomePage::remember(a);WelcomePage::remember(z);home->refresh();home->findChild<QComboBox*>("recentProjectSorting")->setCurrentIndex(1);QCOMPARE(home->findChildren<QPushButton*>("recentProject").first()->property("projectPath").toString(),a);home->findChild<QLineEdit*>("recentProjectSearch")->setText("missing-unmatched");QVERIFY(home->findChildren<QPushButton*>("recentProject").isEmpty());QVERIFY(home->findChild<QLabel*>("noMatchingProjects"));QCOMPARE(QSettings().value("recentProjects").toStringList().size(),2);
    }
};
QTEST_MAIN(Feature128Tests)
#include "feature128_tests.moc"
