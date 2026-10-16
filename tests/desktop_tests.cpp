#include <stdexcept>
#include "mainwindow.hpp"
#include "startpage.hpp"
#include "projectpreview.hpp"
#include "appicons.hpp"
#include <QStyle>
#include <QDialogButtonBox>
#include <QToolBar>
#include <QToolButton>
#include <QDockWidget>
#include "language.hpp"
#include "componentdesign.hpp"
#include <QImage>
#include <QPainter>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QGraphicsPathItem>
#include "codeeditor.hpp"
#include "examples.hpp"
#include <QDoubleSpinBox>
#include <QSlider>
#include <QLabel>
#include <QMenuBar>
#include <QMenu>
#include <QDialog>
#include <QDockWidget>
#include <QTextBrowser>
#include <QTimer>
#include "theme.hpp"
#include <QApplication>
#include <QAction>
#include <QDir>
#include <QFileInfo>
#include <QCheckBox>
#include <QFile>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QScreen>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QSettings>
#include <QtTest>
#ifdef Q_OS_WIN
#define NOMINMAX
#include <windows.h>
#endif
using namespace thinkercad;
class DesktopTests final:public QObject{
    Q_OBJECT
    QTemporaryDir settingsDirectory_;
    static bool write(const QString& path,const QString& text){QFile f(path);return f.open(QIODevice::WriteOnly)&&f.write(text.toUtf8())==text.toUtf8().size();}
    static QPoint port(CircuitCanvas* canvas,const QString& part,const QString& pin){for(auto* item:canvas->scene()->items())if(item->parentItem()&&item->parentItem()->data(1).toString()==part&&item->toolTip().startsWith(pin+" —"))return canvas->mapFromScene(item->scenePos());return {-1000,-1000};}
    static void stop(MainWindow& window){if(window.simulationActive())window.findChild<QAction*>("runSimulation")->trigger();}
private slots:
    void initTestCase(){qputenv("XDG_CACHE_HOME",(settingsDirectory_.path()+"/cache").toUtf8());QApplication::setQuitOnLastWindowClosed(false);QCoreApplication::setOrganizationName("ThinkCadTests");QSettings::setDefaultFormat(QSettings::IniFormat);QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,settingsDirectory_.path());Language::set("de");ComponentDesign::set("blocks");}
    void cleanup(){ComponentDesign::set("blocks");Language::set("de");}
    void analogAcquisitionAndPersistence(){
        MainWindow window;window.show();QVERIFY(window.loadExample("analog_rc"));
        auto* instruments=window.findChild<InstrumentPanel*>("instrumentPanel");QVERIFY(instruments);
        std::string scope,probe,generator;for(const auto& part:window.circuit().parts()){if(part.kind==PartKind::Oscilloscope)scope=part.id;if(part.kind==PartKind::CurrentProbe)probe=part.id;if(part.kind==PartKind::FunctionGenerator)generator=part.id;}
        window.startSimulation();QTRY_VERIFY(instruments->samples(scope).size()>100);QVERIFY(window.simulationActive());QVERIFY(window.circuitState().errors.empty());
        bool charged=false;for(const auto& point:instruments->samples(scope)){QCOMPARE(point.channels.size(),std::size_t(2));if(point.channels[1]&&*point.channels[1]>1)charged=true;}QVERIFY(charged);QVERIFY(instruments->samples(probe).back().channels[0].has_value());
        auto* scopeSelector=window.findChild<QComboBox*>("instrumentSelector");scopeSelector->setCurrentIndex(scopeSelector->findData(QString::fromStdString(scope)));auto* freeze=window.findChild<QCheckBox*>("instrumentFreeze");QVERIFY(freeze);freeze->setChecked(true);const auto snapshot=instruments->csv();const auto before=instruments->samples(scope).size();QTest::qWait(60);QVERIFY(instruments->samples(scope).size()>before);QCOMPARE(instruments->csv(),snapshot);freeze->setChecked(false);
        if(const auto image=qEnvironmentVariable("THINKCAD_ANALOG_SCREENSHOT");!image.isEmpty())QVERIFY(window.grab().save(image));
        window.stopSimulation();const auto count=instruments->samples(scope).size();QTest::qWait(60);QCOMPARE(instruments->samples(scope).size(),count);
        auto* selector=window.findChild<QComboBox*>("instrumentSelector");selector->setCurrentIndex(selector->findData(QString::fromStdString(scope)));const auto csv=instruments->csv();QVERIFY(csv.startsWith("time_s,A_V,B_V\n"));QVERIFY(csv.count('\n')>100);
        window.canvas()->selectPart(generator);auto* amplitude=window.findChild<QDoubleSpinBox*>("generatorAmplitude");amplitude->setValue(1.25);QCOMPARE(window.circuit().part(generator).parameters.at("amplitude"),1.25);
        QTemporaryDir dir;const auto path=dir.filePath("analog.thinkcad");QVERIFY(window.saveProjectTo(path));auto loaded=ProjectIO::load(path);QCOMPARE(loaded.circuit.part(generator).parameters.at("amplitude"),1.25);QCOMPARE(loaded.circuit.part(generator).parameters.at("waveform"),1.0);
        window.resetSimulation();QVERIFY(instruments->samples(scope).empty());
    }
    void instrumentFloatingAndLogicThreshold(){
        CircuitDocument doc;auto scope=doc.addPart(PartKind::Oscilloscope,0,0),logic=doc.addPart(PartKind::LogicAnalyzer,0,0);InstrumentPanel panel;panel.configure(doc);Esp8266Board board;auto state=doc.solve(board);panel.sample(0,state);QVERIFY(!panel.samples(scope).back().channels[0]);QVERIFY(!panel.samples(logic).back().channels[0]);
        doc.connect({logic,"GND"},{"board","GND"});doc.connect({logic,"D0"},{"board","3V3"});doc.part(logic).value=2.5;panel.configure(doc);panel.sample(100,doc.solve(board));QCOMPARE(*panel.samples(logic).back().channels[0],1.0);QVERIFY(!panel.samples(logic).back().channels[1]);
        doc.part(logic).value=4;panel.configure(doc);panel.sample(200,doc.solve(board));QCOMPARE(*panel.samples(logic).back().channels[0],0.0);
        for(unsigned i=0;i<20010;++i)panel.sample(300+i,state);QCOMPARE(panel.samples(logic).size(),std::size_t(20000));
    }
    void componentDesignSelection(){
        MainWindow window;window.show();QTest::qWait(70);auto* canvas=window.canvas();std::string resistor;
        canvas->mutate("Design fixture",[&](auto& doc){resistor=doc.addPart(PartKind::Resistor,430,240);doc.part(resistor).rotation=90;doc.connect({"board","GPIO14"},{resistor,"1"});});
        auto* design=window.findChild<QComboBox*>("componentDesignSelector");QVERIFY(design);QCOMPARE(design->count(),2);
        QCOMPARE(design->itemText(0),QString("Blocks"));QCOMPARE(design->itemText(1),QString("Realistic"));
        QCOMPARE(window.findChild<QLabel*>("componentDesignLabel")->text(),QString("Komponenten Design"));
        QTemporaryDir temporary;const auto beforePath=temporary.filePath("before.thinkcad"),afterPath=temporary.filePath("after.thinkcad");
        QVERIFY(window.saveProjectTo(beforePath));QFile before(beforePath);QVERIFY(before.open(QIODevice::ReadOnly));const auto original=before.readAll();
        const auto count=canvas->history()->count();design->setCurrentIndex(design->findData("realistic"));
        QCOMPARE(ComponentDesign::current(),QString("realistic"));QCOMPARE(QSettings().value("componentDesign").toString(),QString("realistic"));
        QCOMPARE(canvas->history()->count(),count);QVERIFY(canvas->history()->isClean());
        QCOMPARE(window.circuit().part(resistor).rotation,90);QVERIFY(window.saveProjectTo(afterPath));
        QFile after(afterPath);QVERIFY(after.open(QIODevice::ReadOnly));QCOMPARE(after.readAll(),original);
        for(auto* item:canvas->scene()->items())if(item->data(0)=="wire"){
            const auto path=static_cast<QGraphicsPathItem*>(item)->path();QPointF end;
            for(auto* pin:canvas->scene()->items())if(pin->parentItem()&&pin->parentItem()->data(1).toString()==QString::fromStdString(resistor)&&pin->toolTip().startsWith("1 —"))end=pin->scenePos();
            QCOMPARE(QPointF(path.elementAt(path.elementCount()-1).x,path.elementAt(path.elementCount()-1).y),end);
        }
        window.findChild<QComboBox*>("languageSelector")->setCurrentIndex(1);
        QCOMPARE(window.findChild<QLabel*>("componentDesignLabel")->text(),QString("Component Design"));
        {MainWindow second;QCOMPARE(second.findChild<QComboBox*>("componentDesignSelector")->currentData().toString(),QString("realistic"));}
        design->setCurrentIndex(0);QCOMPARE(ComponentDesign::current(),QString("blocks"));QVERIFY(window.circuitState().errors.empty());
    }
    void realisticComponentsRender(){
        ComponentDesign::set("realistic");CircuitDocument document;document.removePart("board");int index=0;double rowTop=0,rowHeight=0;
        auto add=[&](PartKind kind,const std::string& model,double,double){
            if(index&&index%5==0){rowTop+=rowHeight+70;rowHeight=0;}
            const auto id=document.addPart(kind,0,0);auto& part=document.part(id);part.model=model;const auto bounds=ComponentDesign::bounds(part);
            part.x=(index%5)*620+20-bounds.left();part.y=rowTop+20-bounds.top();rowHeight=std::max(rowHeight,bounds.height());++index;return id;
        };
        for(const auto& model:{"esp8266","esp32","esp32-s3","arduino-uno","arduino-nano"})add(PartKind::Esp8266,model,0,0);
        for(const auto& spec:componentCatalog())if(spec.kind!=PartKind::Esp8266){add(spec.kind,{},0,0);}
        add(PartKind::Oled,"ssd1306-128x32",0,1450);
        for(const auto& part:document.parts()){
            const auto bounds=ComponentDesign::bounds(part);std::set<std::pair<double,double>> positions;
            for(const auto& pin:CircuitDocument::pins(part)){
                const auto point=ComponentDesign::pinPosition(part,pin);
                QVERIFY2(bounds.contains(QRectF(point-QPointF(7,7),QSizeF(14,14))),part.id.c_str());
                QVERIFY2(positions.emplace(point.x(),point.y()).second,pin.c_str());
            }
        }
        CircuitCanvas canvas(document);CircuitState state;std::map<std::string,OledDisplay> displays;
        for(const auto& part:document.parts()){
            if(part.kind==PartKind::Led)state.ledCurrent[part.id]=0.01;
            if(part.kind==PartKind::RGBLed){state.ledCurrent[part.id+":R"]=0.015;state.ledCurrent[part.id+":G"]=0.005;}
            if(part.kind==PartKind::Oled){state.voltage[{part.id,"VCC"}]=3.3;state.voltage[{part.id,"GND"}]=0;Frame frame(128,CircuitDocument::displayHeight(part));
                for(int y=4;y<frame.height-4;++y)for(int x=4;x<124;++x)frame.setPixel(x,y,(x/8+y/8)%2==0);
                displays[part.id].present(frame);}
        }
        canvas.updateSimulation(state,displays);QImage image(3100,static_cast<int>(rowTop+rowHeight+80),QImage::Format_ARGB32);image.fill(QColor("#e9eef3"));
        {QPainter painter(&image);canvas.scene()->render(&painter,QRectF(image.rect()),canvas.scene()->itemsBoundingRect().adjusted(-15,-15,15,15));}
        const auto file=qEnvironmentVariable("THINKCAD_TEST_GALLERY");if(!file.isEmpty())QVERIFY(image.save(file));
    }
    void switchDesignDuringSimulation(){
        MainWindow window;window.show();window.loadDemo();window.startSimulation();
        QTRY_VERIFY_WITH_TIMEOUT(window.circuitState().level({"board","GPIO2"})==Level::High,15000);
        auto* design=window.findChild<QComboBox*>("componentDesignSelector");
        design->setCurrentIndex(1);QVERIFY(window.simulationActive());QVERIFY(window.circuitState().errors.empty());
        QTest::qWait(100);design->setCurrentIndex(0);QVERIFY(window.simulationActive());stop(window);
    }
    void toneSketchAndNewComponentsRoundTrip(){
        QTemporaryDir directory;SavedProject project;project.circuit.part("board").model="arduino-uno";
        const auto piezo=project.circuit.addPart(PartKind::PiezoBuzzer,600,100);
        project.circuit.connect({piezo,"+"},{"board","D3"});project.circuit.connect({piezo,"-"},{"board","GND"});
        for(auto kind:{PartKind::Speaker,PartKind::TemperatureSensor,PartKind::BatteryAA,PartKind::ToggleSwitch})project.circuit.addPart(kind,700,400);
        project.sketch.order={"tone.ino"};project.sketch.files["tone.ino"]="void setup(){tone(3,440,500);pinMode(13,OUTPUT);} void loop(){if(millis()>600)digitalWrite(13,HIGH);delay(10);}";
        const auto path=directory.filePath("tone.thinkcad");ProjectIO::save(path,project);const auto loaded=ProjectIO::load(path);QCOMPARE(loaded.circuit.parts().size(),std::size_t(6));
        MainWindow window;QVERIFY(window.openPath(path));window.startSimulation();
        QTRY_VERIFY_WITH_TIMEOUT(window.circuitState().level({"board","D3"})==Level::High,15000);
        QTRY_VERIFY_WITH_TIMEOUT(window.circuitState().level({"board","D3"})==Level::Low,3000);
        QTRY_VERIFY_WITH_TIMEOUT(window.circuitState().level({"board","D13"})==Level::High,3000);
        QVERIFY(window.simulationActive());QVERIFY(window.circuitState().errors.empty());stop(window);
        auto* sound=window.findChild<QAction*>("soundEnabled");QVERIFY(sound);sound->setChecked(false);QVERIFY(!QSettings().value("soundEnabled").toBool());sound->setChecked(true);
    }
    void nativeIconsAndSettingsDialogs(){
        MainWindow window;window.show();QTest::qWait(80);window.resize(1360,850);QTest::qWait(30);
        QCOMPARE(AppIcons::icon(AppIcons::Symbol::Save).pixmap(18,18).toImage(),window.style()->standardIcon(QStyle::SP_DialogSaveButton).pixmap(18,18).toImage());
        for(const auto& id:{"chooseLanguage","chooseComponentDesign"}){
            bool shown=false;QTimer::singleShot(30,&window,[&]{auto* dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());shown=dialog&&dialog->findChild<QComboBox*>()&&dialog->findChild<QDialogButtonBox*>();if(dialog)dialog->reject();});
            auto* action=window.findChild<QAction*>(id);QVERIFY(action);action->trigger();QVERIFY(shown);
        }
        auto* run=window.findChild<QToolButton*>("runButton");auto* reset=window.findChild<QToolButton*>("resetButton");QVERIFY(run&&reset);QVERIFY(run->isVisible()&&reset->isVisible());QVERIFY2(reset->geometry().left()-run->geometry().right()>=10,qPrintable(QString("Run %1,%2 Reset %3,%4").arg(run->x()).arg(run->width()).arg(reset->x()).arg(reset->width())));QCOMPARE(run->height(),reset->height());
        for(const auto* name:{"runButton","resetButton","pauseButton","stepButton","wireButton","wireColorButton"}){
            auto* button=window.findChild<QToolButton*>(name);QVERIFY(button);QCOMPARE(button->size(),run->size());
            QVERIFY(button->icon().isNull());QCOMPARE(button->toolButtonStyle(),Qt::ToolButtonTextOnly);
        }
    }
    void pinNamesAndPhysicalScale(){
        for(const auto& design:{"blocks","realistic"}){
            ComponentDesign::set(design);MainWindow window;window.show();window.canvas()->addPart(PartKind::Led);const auto led=window.circuit().parts().back().id;
            bool anode=false,cathode=false;
            for(auto* item:window.canvas()->scene()->items())if(item->parentItem()&&item->parentItem()->data(1).toString()==QString::fromStdString(led)){anode|=item->toolTip().contains("Anode (+)");cathode|=item->toolTip().contains("Kathode");}
            QVERIFY(anode&&cathode);
        }
        CircuitPart uno{"uno",PartKind::Esp8266};uno.model="arduino-uno";CircuitPart led{"led",PartKind::Led};CircuitPart resistor{"r",PartKind::Resistor};CircuitPart breadboard{"bb",PartKind::Breadboard30};
        QVERIFY(ComponentDesign::bounds(led).width()<ComponentDesign::bounds(uno).width()/3);
        QVERIFY(ComponentDesign::bounds(resistor).width()<ComponentDesign::bounds(uno).width()/3);
        QCOMPARE(QLineF(ComponentDesign::pinPosition(breadboard,"a1"),ComponentDesign::pinPosition(breadboard,"a2")).length(),15.24);
        CircuitPart oled{"oled",PartKind::Oled};QVERIFY(std::abs(QLineF(ComponentDesign::pinPosition(oled,"GND"),ComponentDesign::pinPosition(oled,"VCC")).length()-15.24)<0.0001);
    }
    void separateBoardSketchesRunTogether(){
        MainWindow window;window.show();std::string second;
        window.canvas()->mutate("Two boards",[&](auto& doc){doc.part("board").model="arduino-uno";second=doc.addPart(PartKind::Esp8266,450,100);doc.part(second).model="arduino-uno";doc.connect({"board","D2"},{second,"D3"});doc.connect({"board","GND"},{second,"GND"});});
        auto* boards=window.findChild<QComboBox*>("sketchBoardSelector");QVERIFY(boards);QCOMPARE(boards->count(),2);
        auto code=[&](){return qobject_cast<QPlainTextEdit*>(window.findChild<QTabWidget*>("codeTabs")->currentWidget());};
        const QString sender="void setup(){pinMode(2,OUTPUT);pinMode(13,OUTPUT);digitalWrite(13,LOW);} void loop(){digitalWrite(2,(millis()/200)%2?HIGH:LOW);delay(10);}";
        code()->setPlainText(sender);code()->document()->setModified(true);
        boards->setCurrentIndex(boards->findData(QString::fromStdString(second)));
        QVERIFY(window.sketchText()!=sender);
        const QString receiver="void setup(){pinMode(3,INPUT_PULLUP);pinMode(13,OUTPUT);} void loop(){digitalWrite(13,digitalRead(3));delay(10);}";
        code()->setPlainText(receiver);code()->document()->setModified(true);
        boards->setCurrentIndex(boards->findData("board"));QVERIFY(window.sketchText().contains("pinMode(2,OUTPUT)"));
        QTemporaryDir dir;const auto file=dir.filePath("two.thinkcad");QVERIFY(window.saveProjectTo(file));QVERIFY(window.openPath(file));
        boards->setCurrentIndex(boards->findData(QString::fromStdString(second)));QVERIFY(window.sketchText().contains("digitalRead(3)"));
        window.startSimulation();auto* console=window.findChild<QPlainTextEdit*>("consoleOutput");QTRY_VERIFY_WITH_TIMEOUT(console->toPlainText().count("setup() abgeschlossen")==2,20000);QVERIFY(window.simulationActive());
        QTRY_VERIFY_WITH_TIMEOUT(window.circuitState().level({second,"D13"})==Level::High,4000);
        QTRY_VERIFY_WITH_TIMEOUT(window.circuitState().level({second,"D13"})==Level::Low,4000);
        QCOMPARE(window.circuitState().level({"board","D13"}),Level::Low);QVERIFY(window.simulationActive());
        boards->setCurrentIndex(boards->findData("board"));QVERIFY(window.sketchText().contains("pinMode(2,OUTPUT)"));QVERIFY(window.simulationActive());
        window.resetSimulation();QTRY_VERIFY_WITH_TIMEOUT(console->toPlainText().count("setup() abgeschlossen")==4,20000);QVERIFY(window.simulationActive());stop(window);
    }
    void conflictingBoardsStopTogether(){
        SavedProject project;project.circuit.part("board").model="arduino-uno";const auto other=project.circuit.addPart(PartKind::Esp8266,450,100);project.circuit.part(other).model="arduino-uno";project.circuit.connect({"board","D2"},{other,"D2"});
        auto sketch=[](const QString& level){SketchProject s;s.order={"sketch.ino"};s.files["sketch.ino"]="void setup(){pinMode(2,OUTPUT);digitalWrite(2,"+level+");}void loop(){delay(10);}";return s;};project.sketch=sketch("HIGH");project.boardSketches["board"]=project.sketch;project.boardSketches[QString::fromStdString(other)]=sketch("LOW");
        QTemporaryDir dir;const auto file=dir.filePath("conflict.thinkcad");ProjectIO::save(file,project);MainWindow window;window.show();QVERIFY(window.openPath(file));window.startSimulation();auto* console=window.findChild<QPlainTextEdit*>("consoleOutput");
        QTRY_VERIFY_WITH_TIMEOUT(console->toPlainText().contains("Kurzschluss"),20000);QTRY_VERIFY(!window.simulationActive());
    }
    void runShortcut(){
        MainWindow window;window.show();window.loadDemo();QTest::qWait(80);
        auto* run=window.findChild<QAction*>("runSimulation");
        QVERIFY(run->shortcuts().contains(QKeySequence(Qt::CTRL|Qt::Key_R)));
        QVERIFY(run->shortcuts().contains(QKeySequence(Qt::Key_F5)));
        window.canvas()->setFocus();QTest::keyClick(window.canvas(),Qt::Key_R,Qt::ControlModifier);
        QTRY_VERIFY_WITH_TIMEOUT(window.circuitState().level({"board","GPIO2"})==Level::High,15000);
        QVERIFY(window.simulationActive());
        QTest::keyClick(window.canvas(),Qt::Key_R,Qt::ControlModifier);
        QTRY_VERIFY(!window.simulationActive());
    }
    void rotationShortcutAndPersistence(){
        MainWindow window;window.show();QTest::qWait(80);auto* canvas=window.canvas();std::string resistor;
        canvas->mutate("Rotation fixture",[&](auto& doc){resistor=doc.addPart(PartKind::Resistor,400,240);doc.connect({"board","GPIO14"},{resistor,"1"});const_cast<CircuitWire&>(doc.wires().back()).points={{330,460}};});
        auto partItem=[&]()->QGraphicsItem*{for(auto* item:canvas->scene()->items())if(item->data(0)=="part"&&item->data(1).toString()==QString::fromStdString(resistor))return item;return nullptr;};
        auto pinPosition=[&](){for(auto* item:canvas->scene()->items())if(item->parentItem()==partItem()&&item->toolTip().startsWith("1 —"))return item->scenePos();return QPointF(-10000,-10000);};
        const auto center=partItem()->mapToScene(partItem()->boundingRect().center());const auto originalPin=pinPosition();
        canvas->selectPart(resistor);canvas->setFocus();QTest::keyClick(canvas,Qt::Key_R);
        QCOMPARE(window.circuit().part(resistor).rotation,90);QCOMPARE(partItem()->rotation(),90.0);
        QCOMPARE(partItem()->mapToScene(partItem()->boundingRect().center()),center);QVERIFY(pinPosition()!=originalPin);
        for(auto* item:canvas->scene()->items())if(item->data(0)=="wire"){
            const auto path=static_cast<QGraphicsPathItem*>(item)->path();QCOMPARE(path.elementCount(),3);
            QCOMPARE(QPointF(path.elementAt(2).x,path.elementAt(2).y),pinPosition());
            QCOMPARE(QPointF(path.elementAt(1).x,path.elementAt(1).y),QPointF(330,460));
        }
        canvas->history()->undo();QCOMPARE(window.circuit().part(resistor).rotation,0);QCOMPARE(pinPosition(),originalPin);
        canvas->history()->redo();QCOMPARE(window.circuit().part(resistor).rotation,90);
        QTemporaryDir dir;const auto file=dir.filePath("rotated.thinkcad");QVERIFY(window.saveProjectTo(file));QVERIFY(window.openPath(file));
        QCOMPARE(window.circuit().part(resistor).rotation,90);QCOMPARE(partItem()->rotation(),90.0);
        canvas->selectPart(resistor);canvas->setFocus();for(int i=0;i<3;++i)QTest::keyClick(canvas,Qt::Key_R);
        QCOMPARE(window.circuit().part(resistor).rotation,0);QCOMPARE(pinPosition(),originalPin);
        canvas->setEditingEnabled(false);QTest::keyClick(canvas,Qt::Key_R);QCOMPARE(window.circuit().part(resistor).rotation,0);canvas->setEditingEnabled(true);
        // Typing R in the sketch editor must never rotate a selected component.
        window.findChild<QDockWidget*>("codeDock")->show();auto* editor=static_cast<CodeEditor*>(window.findChild<QTabWidget*>("codeTabs")->currentWidget());
        editor->setFocus();editor->moveCursor(QTextCursor::End);QTest::keyClicks(editor,"r");
        QVERIFY(editor->toPlainText().endsWith("r"));QCOMPARE(window.circuit().part(resistor).rotation,0);
        canvas->setFocus();for(auto* item:canvas->scene()->items())if(item->data(0)=="part")item->setSelected(true);
        QTest::keyClick(canvas,Qt::Key_R);QCOMPARE(window.circuit().part(resistor).rotation,90);QCOMPARE(window.circuit().part("board").rotation,90);
        QTest::keyClick(canvas,Qt::Key_R);QCOMPARE(window.circuit().part(resistor).rotation,180);QCOMPARE(window.circuit().part("board").rotation,180);
    }
    void componentSearchAndBoards(){
        MainWindow window;window.show();QTest::qWait(60);
        QVERIFY(!window.findChild<QComboBox*>("boardProfile"));
        auto* search=window.findChild<QLineEdit*>("componentSearch");QVERIFY(search);
        search->setText("nAnO");
        QPushButton* nano=nullptr;
        int shown=0;
        for(auto* card:window.findChildren<QPushButton*>("componentCard"))if(!card->isHidden()){
            ++shown;if(card->property("model")=="arduino-nano")nano=card;
        }
        QCOMPARE(shown,1);QVERIFY(nano);
        QTest::mouseClick(nano,Qt::LeftButton);
        QCOMPARE(window.circuit().parts().size(),std::size_t(2));
        const auto id=window.circuit().parts().back().id;
        QCOMPARE(window.circuit().part(id).model,std::string("arduino-nano"));
        window.canvas()->selectPart(id);
        window.findChild<QPushButton*>("activateBoard")->click();
        QCOMPARE(window.circuit().boardId(),id);
        QTemporaryDir dir;const auto saved=dir.filePath("boards.thinkcad");
        QVERIFY(window.saveProjectTo(saved));QVERIFY(window.openPath(saved));
        QCOMPARE(window.circuit().boardId(),id);
        window.canvas()->selectPart("board");window.canvas()->removeSelected();
        QCOMPARE(window.circuit().parts().size(),std::size_t(1));
        window.canvas()->selectPart(id);window.canvas()->removeSelected();
        QVERIFY(window.circuit().parts().empty());QVERIFY(window.circuitState().errors.empty());
        QVERIFY(window.saveProjectTo(saved));QVERIFY(window.openPath(saved));QVERIFY(window.circuit().parts().empty());
        window.canvas()->history()->clear();search->clear();
        for(auto* card:window.findChildren<QPushButton*>("componentCard"))if(card->property("model")=="arduino-uno")card->click();
        QCOMPARE(window.circuit().parts().size(),std::size_t(1));QVERIFY(!window.circuit().boardId().empty());
        window.canvas()->history()->undo();QVERIFY(window.circuit().parts().empty());
        window.canvas()->history()->redo();QCOMPARE(window.circuit().parts().size(),std::size_t(1));
    }
    void languageSelection(){
        MainWindow window;window.show();
        auto* language=window.findChild<QComboBox*>("languageSelector");QVERIFY(language);
        auto* search=window.findChild<QLineEdit*>("componentSearch");QVERIFY(search);
        language->setCurrentIndex(language->findData("en"));
        QCOMPARE(window.menuBar()->actions().front()->text(),QString("File"));
        QCOMPARE(search->placeholderText(),QString("Search components…"));
        QCOMPARE(Language::current(),QString("en"));QSettings settings;settings.sync();QCOMPARE(settings.value("language").toString(),QString("en"));
        search->setText("resistor");int matches=0;
        for(auto* card:window.findChildren<QPushButton*>("componentCard"))if(!card->isHidden()&&card->property("kind").toInt()==int(PartKind::Resistor))++matches;
        QCOMPARE(matches,1);
        {MainWindow second;QCOMPARE(second.findChild<QComboBox*>("languageSelector")->currentData().toString(),QString("en"));}
        language->setCurrentIndex(language->findData("de"));
        QCOMPARE(window.menuBar()->actions().front()->text(),QString("Datei"));
        QCOMPARE(search->placeholderText(),QString("Komponenten suchen…"));
        QCOMPARE(Language::text("Widerstand"),QString("Widerstand"));
    }
    void arduinoBoards_data(){
        QTest::addColumn<QString>("model");QTest::addColumn<QString>("analogPin");
        QTest::newRow("uno-a0")<<QString("arduino-uno")<<QString("A0");
        QTest::newRow("nano-a7")<<QString("arduino-nano")<<QString("A7");
        QTest::newRow("nano-channel-7")<<QString("arduino-nano")<<QString("7");
    }
    void arduinoBoards(){
        QFETCH(QString,model);QFETCH(QString,analogPin);
        QTemporaryDir dir;const auto path=dir.filePath("arduino.ino");
        QVERIFY(write(path,"void setup(){pinMode(LED_BUILTIN,OUTPUT);}\nvoid loop(){digitalWrite(LED_BUILTIN,analogRead("+analogPin+")>=510?HIGH:LOW);delay(10);}\n"));
        MainWindow window;window.show();QVERIFY(window.openPath(path));
        std::string board,led;
        window.canvas()->mutate("Arduino circuit",[&](auto& doc){
            doc.removePart("board");board=doc.addPart(PartKind::Esp8266,80,80);doc.part(board).model=model.toStdString();
            const auto source=doc.addPart(PartKind::AnalogSource,350,80);doc.part(source).value=2.5;
            doc.connect({source,"OUT"},{board,model=="arduino-uno"?"A0":"A7"});doc.connect({source,"GND"},{board,"GND"});
            const auto resistor=doc.addPart(PartKind::Resistor,350,250);led=doc.addPart(PartKind::Led,550,250);
            doc.connect({board,"D13"},{resistor,"1"});doc.connect({resistor,"2"},{led,"A"});doc.connect({led,"K"},{board,"GND"});
        });
        QVERIFY(window.circuitState().errors.empty());window.startSimulation();
        QTRY_VERIFY_WITH_TIMEOUT(window.circuitState().level({board,"D13"})==Level::High,15000);
        QVERIFY(window.simulationActive());QCOMPARE(*window.circuitState().voltage.at({board,"D13"}),5.0);
        QVERIFY(window.circuitState().ledCurrent.at(led)>0.012);stop(window);
        QVERIFY(window.saveProjectTo(dir.filePath("arduino.thinkcad")));QVERIFY(window.openPath(dir.filePath("arduino.thinkcad")));
        QCOMPARE(window.circuit().part(board).model,model.toStdString());
    }
    void recentPreviewsFollowProjectChanges(){
        QTemporaryDir dir;const auto path=dir.filePath("preview.thinkcad");SavedProject project;project.sketch.order={"sketch.ino"};project.sketch.files["sketch.ino"]="void setup(){} void loop(){}";
        project.circuit.addPart(PartKind::Led,0,0);ProjectIO::save(path,project);
        const auto first=ProjectPreview::load(path);QVERIFY(!first.isNull());QCOMPARE(first.size(),QSize(320,192));
        QCOMPARE(ProjectPreview::load(path).convertToFormat(QImage::Format_ARGB32),first.convertToFormat(QImage::Format_ARGB32));
        // An external edit must invalidate a previously cached screenshot.
        project.circuit.addPart(PartKind::Resistor,250,120);ProjectIO::save(path,project);
        const auto updated=ProjectPreview::load(path);QVERIFY(!updated.isNull());QVERIFY(updated!=first);
        QVERIFY(write(path,"invalid project"));QVERIFY(ProjectPreview::load(path).isNull());
        QVERIFY(QFile::remove(path));QVERIFY(ProjectPreview::load(path).isNull());
    }
    void startPageNavigationAndRecentProjects(){
        Theme::apply(false);
        QSettings settings;const auto original=settings.value("recentProjects");
        const auto originalDark=settings.value("darkMode",false).toBool();
        settings.remove("recentProjects");QTemporaryDir dir;
        MainWindow window;window.show();QTest::qWait(80);
        window.canvas()->addPart(PartKind::Resistor);const auto parts=window.circuit().parts().size();
        const auto saved=dir.filePath("My circuit.thinkcad");QVERIFY(window.saveProjectTo(saved));
        auto* editor=window.findChild<QTabWidget*>("codeTabs")->currentWidget();
        auto* code=qobject_cast<QPlainTextEdit*>(editor);QVERIFY(code);code->appendPlainText("// keep this unsaved edit");const auto sketch=window.sketchText();
        auto* codeDock=window.findChild<QDockWidget*>("codeDock");codeDock->hide();
        window.showStartPage();QTest::qWait(50);QVERIFY(window.startPageVisible());
        for(auto* dock:window.findChildren<QDockWidget*>())QVERIFY(dock->isHidden());
        auto* home=window.findChild<WelcomePage*>();QVERIFY(home);
        for(auto* action:home->findChildren<QPushButton*>())QVERIFY2(action->icon().isNull(),qPrintable(action->objectName()));
        const auto recent=home->findChildren<QPushButton*>("recentProject");QCOMPARE(recent.size(),1);QCOMPARE(recent.front()->property("projectPath").toString(),QFileInfo(saved).canonicalFilePath());
        auto* thumbnail=home->findChild<QLabel*>("recentProjectPreview");QVERIFY(thumbnail);QTRY_VERIFY(!thumbnail->pixmap().isNull());
        const auto capture=qEnvironmentVariable("THINKCAD_UI_CAPTURE_DIR");if(!capture.isEmpty())window.grab().save(capture+"/home-light.png");
        home->findChild<QPushButton*>("settingsNavigation")->click();auto* dark=home->findChild<QCheckBox*>("startDarkMode");QVERIFY(dark);dark->setChecked(true);QVERIFY(settings.value("darkMode").toBool());
        auto chooseLanguage=[&](const QString& value){QTimer::singleShot(30,&window,[value]{auto* dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());if(dialog){auto* choice=dialog->findChild<QComboBox*>();choice->setCurrentIndex(choice->findData(value));dialog->accept();}});home->findChild<QPushButton*>("startChooseLanguage")->click();};chooseLanguage("en");QCOMPARE(home->findChild<QPushButton*>("homeNavigation")->text(),QString("My projects"));chooseLanguage("de");
        home->findChild<QPushButton*>("homeNavigation")->click();if(!capture.isEmpty())window.grab().save(capture+"/home-dark.png");
        home->findChild<QPushButton*>("resumeEditor")->click();QVERIFY(!window.startPageVisible());QCOMPARE(window.circuit().parts().size(),parts);QCOMPARE(window.sketchText(),sketch);QVERIFY(codeDock->isHidden());
        auto* run=window.findChild<QToolButton*>("runButton");auto* reset=window.findChild<QToolButton*>("resetButton");QVERIFY(run&&reset);QVERIFY(reset->icon().isNull());QCOMPARE(run->height(),reset->height());
        // Clear the edit before opening the recent project, so no save prompt is needed.
        code->document()->setModified(false);window.showStartPage();home->findChild<QPushButton*>("recentProject")->click();QVERIFY(!window.startPageVisible());QVERIFY(!window.sketchText().contains("keep this unsaved edit"));QVERIFY(!codeDock->isHidden());
        window.showStartPage();home->findChild<QPushButton*>("clearRecentProjects")->click();QVERIFY(settings.value("recentProjects").toStringList().isEmpty());QVERIFY(!home->findChild<QPushButton*>("clearRecentProjects")->isEnabled());QVERIFY(QFileInfo::exists(saved));
        WelcomePage::remember(saved);home->refresh();
        home->findChild<QPushButton*>("removeRecentProject")->click();
        QTRY_VERIFY(home->findChildren<QPushButton*>("recentProject").isEmpty());
        QVERIFY(QFileInfo::exists(saved));
        const auto missing=dir.filePath("Missing.thinkcad");settings.setValue("recentProjects",QStringList{missing});home->refresh();
        QVERIFY(!home->findChild<QPushButton*>("recentProject")->isEnabled());
        home->findChild<QPushButton*>("clearRecentProjects")->click();
        home->findChild<QPushButton*>("startNewProject")->click();QVERIFY(!window.startPageVisible());QCOMPARE(window.circuit().parts().size(),std::size_t(1));
        window.findChild<QAction*>("darkModeAction")->setChecked(originalDark);settings.setValue("recentProjects",original);
    }
    void resetRestartsSetupAndClearsStoppedState(){
        QTemporaryDir dir;const auto path=dir.filePath("reset.ino");
        QVERIFY(write(path,"void setup(){pinMode(D5,OUTPUT);digitalWrite(D5,HIGH); } void loop(){delay(100);}"));
        MainWindow window;window.show();QVERIFY(window.loadExample("led_blink"));QVERIFY(window.openPath(path));
        const auto parts=window.circuit().parts().size(),wires=window.circuit().wires().size();const auto sketch=window.sketchText();
        auto* console=window.findChild<QPlainTextEdit*>("consoleOutput");auto* reset=window.findChild<QAction*>("resetSimulation");QVERIFY(reset);window.startSimulation();
        QTRY_VERIFY_WITH_TIMEOUT(console->toPlainText().count("setup() abgeschlossen")==1,15000);QVERIFY(window.simulationActive());
        QTRY_VERIFY(window.circuitState().level({"board","GPIO14"})==Level::High);
        reset->trigger();QTRY_VERIFY_WITH_TIMEOUT(console->toPlainText().count("setup() abgeschlossen")==2,15000);QVERIFY(window.simulationActive());
        QCOMPARE(window.sketchText(),sketch);QCOMPARE(window.circuit().parts().size(),parts);QCOMPARE(window.circuit().wires().size(),wires);
        stop(window);reset->trigger();QVERIFY(!window.simulationActive());QVERIFY(window.circuitState().level({"board","GPIO14"})!=Level::High);
        for(const auto& [id,current]:window.circuitState().ledCurrent){Q_UNUSED(id);QVERIFY(current<0.00001);}
    }
    void normalWindowControls(){MainWindow window;window.show();QTest::qWait(60);QVERIFY(window.windowTitle().contains("ThinkCad"));QVERIFY(!window.windowTitle().contains("ThinkerCad"));const auto flags=window.windowFlags();QVERIFY(flags&Qt::WindowCloseButtonHint);QVERIFY(flags&Qt::WindowMinimizeButtonHint);QVERIFY(flags&Qt::WindowMaximizeButtonHint);QVERIFY(!(flags&Qt::FramelessWindowHint));QVERIFY(!window.windowIcon().isNull());window.showMinimized();QTRY_VERIFY(window.isMinimized());window.showMaximized();QTRY_VERIFY(window.isMaximized());window.showNormal();QTRY_VERIFY(!window.isMaximized()&&!window.isMinimized());QVERIFY(window.close());QVERIFY(!window.isVisible());}
    void nativeWindowChrome(){
#ifdef Q_OS_WIN
        if(QApplication::platformName()!="windows")QSKIP("Native chrome is checked separately with QPA=windows.");MainWindow window;window.show();QTest::qWait(150);const auto hwnd=reinterpret_cast<HWND>(window.winId());const auto style=GetWindowLongPtrW(hwnd,GWL_STYLE);QVERIFY((style&WS_CAPTION)==WS_CAPTION);QVERIFY(style&WS_SYSMENU);QVERIFY(style&WS_MINIMIZEBOX);QVERIFY(style&WS_MAXIMIZEBOX);QVERIFY(style&WS_THICKFRAME);const auto available=window.screen()->availableGeometry();QVERIFY(window.frameGeometry().width()<=available.width());QVERIFY(window.frameGeometry().height()<=available.height());SendMessageW(hwnd,WM_SYSCOMMAND,SC_MINIMIZE,0);QTRY_VERIFY(window.isMinimized());SendMessageW(hwnd,WM_SYSCOMMAND,SC_MAXIMIZE,0);QTRY_VERIFY(window.isMaximized());SendMessageW(hwnd,WM_SYSCOMMAND,SC_RESTORE,0);QTRY_VERIFY(!window.isMinimized()&&!window.isMaximized());SendMessageW(hwnd,WM_SYSCOMMAND,SC_CLOSE,0);QTRY_VERIFY(!window.isVisible());
#else
        QSKIP("Win32 window chrome requires Windows.");
#endif
    }
    void mouseWiringAndUndo(){MainWindow window;window.show();QTest::qWait(80);auto* canvas=window.canvas();canvas->addPart(PartKind::Oled);const auto oled=window.circuit().parts().back().id;canvas->fitCircuit();QTest::qWait(30);QTest::mouseClick(canvas->viewport(),Qt::LeftButton,Qt::NoModifier,port(canvas,"board","3V3"));QTest::mouseClick(canvas->viewport(),Qt::LeftButton,Qt::NoModifier,port(canvas,QString::fromStdString(oled),"VCC"));QTRY_COMPARE(window.circuit().wires().size(),std::size_t(1));QVERIFY(window.circuit().connected({"board","3V3"},{oled,"VCC"}));for(auto* item:canvas->scene()->items())if(item->data(0)=="wire")QCOMPARE(static_cast<QGraphicsPathItem*>(item)->path().elementCount(),2);for(auto* item:canvas->scene()->items())if(item->data(0)=="wire")item->setSelected(true);canvas->setFocus();QTest::keyClick(canvas,Qt::Key_Delete);QTRY_COMPARE(window.circuit().wires().size(),std::size_t(0));canvas->history()->undo();QCOMPARE(window.circuit().wires().size(),std::size_t(1));canvas->history()->redo();QCOMPARE(window.circuit().wires().size(),std::size_t(0));}
    void importInoTabsAndPortableProject(){QTemporaryDir temporary;QDir root(temporary.path());root.mkdir("Blink");const auto dir=root.filePath("Blink");const auto primary=dir+"/Blink.ino";const QString original="#include \"Counter.h\"\nvoid setup(){pinMode(D5,OUTPUT);}\nvoid loop(){digitalWrite(D5,stateFor(21));delay(20);}\n";QVERIFY(write(primary,original));QVERIFY(write(dir+"/helpers.ino","int stateFor(int value){return doubled(value)==42?HIGH:LOW;}\n"));QVERIFY(write(dir+"/Counter.h","#pragma once\nint doubled(int value);\n"));QVERIFY(write(dir+"/Counter.cpp","#include \"Counter.h\"\nint doubled(int value){return value*2;}\n"));MainWindow window;window.show();QVERIFY(window.openPath(primary));QCOMPARE(window.findChild<QTabWidget*>("codeTabs")->count(),4);window.startSimulation();QTRY_VERIFY_WITH_TIMEOUT(window.circuitState().level({"board","GPIO14"})==Level::High,15000);stop(window);QFile unchanged(primary);QVERIFY(unchanged.open(QIODevice::ReadOnly));QCOMPARE(QString::fromUtf8(unchanged.readAll()),original);unchanged.close();const auto saved=root.filePath("Blink.thinkercad");QVERIFY(window.saveProjectTo(saved));QVERIFY(QDir(dir).removeRecursively());QVERIFY(window.openPath(saved));QCOMPARE(window.findChild<QTabWidget*>("codeTabs")->count(),4);window.startSimulation();QTRY_VERIFY_WITH_TIMEOUT(window.circuitState().level({"board","GPIO14"})==Level::High,15000);stop(window);}
    void wiredButtonChangesLed(){QTemporaryDir dir;const auto path=dir.filePath("button.ino");QVERIFY(write(path,"void setup(){pinMode(D6,INPUT_PULLUP);pinMode(D5,OUTPUT);}\nvoid loop(){digitalWrite(D5,digitalRead(D6)==LOW?HIGH:LOW);delay(40);}\n"));MainWindow window;window.show();QVERIFY(window.openPath(path));std::string led,resistor,button;window.canvas()->mutate("Test circuit",[&](auto& doc){led=doc.addPart(PartKind::Led,430,150);resistor=doc.addPart(PartKind::Resistor,270,150);button=doc.addPart(PartKind::Button,270,300);doc.connect({"board","GPIO14"},{resistor,"1"});doc.connect({resistor,"2"},{led,"A"});doc.connect({led,"K"},{"board","GND"});doc.connect({button,"1"},{"board","GPIO12"});doc.connect({button,"2"},{"board","GND"});});window.startSimulation();auto* output=window.findChild<QPlainTextEdit*>("consoleOutput");QTRY_VERIFY_WITH_TIMEOUT(output->toPlainText().contains("setup() abgeschlossen"),15000);QTRY_VERIFY(window.circuitState().ledCurrent.at(led)<1e-6);window.canvas()->selectPart(button);auto* pressed=window.findChild<QCheckBox*>("componentPressed");QVERIFY(pressed&&pressed->isVisible());QTest::mouseClick(pressed,Qt::LeftButton);QTRY_VERIFY_WITH_TIMEOUT(window.circuitState().ledCurrent.at(led)>0.005,3000);QTest::mouseClick(pressed,Qt::LeftButton);QTRY_VERIFY_WITH_TIMEOUT(window.circuitState().ledCurrent.at(led)<1e-6,3000);stop(window);}
    void closeDuringSimulation(){MainWindow window;window.loadDemo();window.show();window.startSimulation();QTRY_VERIFY_WITH_TIMEOUT(window.circuitState().level({"board","GPIO2"})==Level::High,15000);QElapsedTimer timer;timer.start();QVERIFY(window.close());QVERIFY(!window.simulationActive());QVERIFY(!window.isVisible());QVERIFY(timer.elapsed()<2500);}
    void disconnectedOledFailsBegin(){QTemporaryDir dir;const auto path=dir.filePath("oled.ino");QVERIFY(write(path,"#include <Adafruit_SSD1306.h>\nAdafruit_SSD1306 display(128,64,&Wire,-1);\nvoid setup(){Wire.begin(D2,D1);if(!display.begin(SSD1306_SWITCHCAPVCC,0x3c))Serial.println(\"missing OLED\");}\nvoid loop(){delay(30);}\n"));MainWindow window;window.show();QVERIFY(window.openPath(path));window.startSimulation();auto* console=window.findChild<QPlainTextEdit*>("consoleOutput");QTRY_VERIFY_WITH_TIMEOUT(console->toPlainText().contains("missing OLED"),15000);QVERIFY(console->toPlainText().contains("OLED nicht erreichbar"));stop(window);}
    void circuitProjectRoundTrip(){QTemporaryDir dir;SavedProject p;p.sketch.name="Wiring";p.sketch.order={"Wiring.ino"};p.sketch.files["Wiring.ino"]="void setup(){} void loop(){delay(20);}";const auto resistor=p.circuit.addPart(PartKind::Resistor,310,170);p.circuit.part(resistor).value=470;p.circuit.part(resistor).color="#22c55e";p.circuit.connect({"board","GPIO14"},{resistor,"1"},"#ec5967");const auto path=dir.filePath("Wiring.thinkercad");ProjectIO::save(path,p);const auto loaded=ProjectIO::load(path);QCOMPARE(loaded.circuit.parts().size(),p.circuit.parts().size());QCOMPARE(loaded.circuit.part(resistor).x,310.0);QCOMPARE(loaded.circuit.part(resistor).value,470.0);QCOMPARE(QString::fromStdString(loaded.circuit.part(resistor).color),QString("#22c55e"));QVERIFY(loaded.circuit.connected({"board","GPIO14"},{resistor,"1"}));QCOMPARE(QString::fromStdString(loaded.circuit.wires().front().color),QString("#ec5967"));QCOMPARE(loaded.sketch.files,p.sketch.files);}
    void simulationSpeedSlider(){MainWindow window;window.show();QTest::qWait(50);auto* speed=window.findChild<QSlider*>("simulationSpeed");auto* label=window.findChild<QLabel*>("simulationSpeedLabel");QVERIFY(speed&&label);QCOMPARE(speed->value(),40);speed->setFocus();QTest::keyClick(speed,Qt::Key_Home);QCOMPARE(label->text(),QString("0,10×"));QTest::keyClick(speed,Qt::Key_End);QCOMPARE(label->text(),QString("10,00×"));speed->setValue(40);QCOMPARE(label->text(),QString("1,00×"));}
    void examplesCompileAndSimulate_data(){QTest::addColumn<QString>("id");for(const auto& example:ExampleCatalog::entries())QTest::newRow(example.id.toUtf8().constData())<<example.id;}
    void examplesCompileAndSimulate(){QFETCH(QString,id);MainWindow window;window.show();QVERIFY(window.loadExample(id));QVERIFY(window.circuitState().errors.empty());QVERIFY(!window.circuit().wires().empty());window.startSimulation();if(id.startsWith("lcd_")||id=="servo_sweep"){auto* console=window.findChild<QPlainTextEdit*>("consoleOutput");QTRY_VERIFY_WITH_TIMEOUT(console->toPlainText().contains("setup() abgeschlossen"),15000);QVERIFY(window.simulationActive());QVERIFY(window.circuitState().errors.empty());stop(window);return;}if(id=="dc_motor"){QTest::qWait(100);QVERIFY(window.simulationActive());QVERIFY(window.circuitState().errors.empty());stop(window);return;}if(id.startsWith("analog_")||id=="pwm_filter"){std::string scope;for(const auto& part:window.circuit().parts())if(part.kind==PartKind::Oscilloscope)scope=part.id;auto* panel=window.findChild<InstrumentPanel*>("instrumentPanel");QTRY_VERIFY_WITH_TIMEOUT(panel->samples(scope).size()>100,15000);QVERIFY(window.simulationActive());QVERIFY(window.circuitState().errors.empty());QVERIFY(panel->samples(scope).back().channels[0]);QVERIFY(panel->samples(scope).back().channels[1]);stop(window);return;}auto* console=window.findChild<QPlainTextEdit*>("consoleOutput");QTRY_VERIFY_WITH_TIMEOUT(console->toPlainText().contains("setup() abgeschlossen"),15000);QVERIFY(window.simulationActive());if(id=="animation"||id=="two_oleds"||id=="oled_text_32"){QTest::qWait(250);QVERIFY(window.simulationActive());QVERIFY(!console->toPlainText().contains("OLED nicht erreichbar"));}else if(id=="sound_melody"){QTRY_VERIFY(window.circuitState().level({"board","D3"})==Level::High);}else if(id!="button_led"&&id!="ir_remote"&&id!="nfc_reader"&&id!="nrf24_pair"&&id!="ask_pair"){QTRY_VERIFY(std::any_of(window.circuitState().ledCurrent.begin(),window.circuitState().ledCurrent.end(),[](const auto& pair){return pair.second>0.001;}));}stop(window);}
    void manualWireBends(){MainWindow window;window.show();QTest::qWait(80);auto* canvas=window.canvas();canvas->addPart(PartKind::Oled);const auto oled=window.circuit().parts().back().id;canvas->fitCircuit();QTest::qWait(30);QTest::mouseClick(canvas->viewport(),Qt::LeftButton,Qt::NoModifier,port(canvas,"board","3V3"));QTest::mouseClick(canvas->viewport(),Qt::LeftButton,Qt::NoModifier,QPoint(25,canvas->viewport()->height()-25));QTest::mouseClick(canvas->viewport(),Qt::LeftButton,Qt::NoModifier,port(canvas,QString::fromStdString(oled),"VCC"));QTRY_COMPARE(window.circuit().wires().size(),std::size_t(1));QCOMPARE(window.circuit().wires().front().points.size(),std::size_t(1));for(auto* item:canvas->scene()->items())if(item->data(0)=="wire"){const auto path=static_cast<QGraphicsPathItem*>(item)->path();QCOMPARE(path.elementCount(),3);QVERIFY(path.elementAt(0).x!=path.elementAt(1).x);QVERIFY(path.elementAt(0).y!=path.elementAt(1).y);const auto bend=window.circuit().wires().front().points.front();QCOMPARE(path.elementAt(1).x,bend.x);QCOMPARE(path.elementAt(1).y,bend.y);}const auto id=window.circuit().wires().front().id;const auto old=window.circuit().wires().front().points.front();canvas->moveBend(id,0,{old.x+40,old.y-30});QCOMPARE(window.circuit().wires().front().points.front().x,old.x+40);canvas->history()->undo();QCOMPARE(window.circuit().wires().front().points.front().x,old.x);QTemporaryDir dir;QVERIFY(window.saveProjectTo(dir.filePath("bends.thinkcad")));QVERIFY(window.openPath(dir.filePath("bends.thinkcad")));QCOMPARE(window.circuit().wires().front().points.size(),std::size_t(1));QCOMPARE(window.circuit().wires().front().points.front().x,old.x);}
    void mouseDragBend(){
        MainWindow window;window.show();auto* canvas=window.canvas();
        canvas->mutate("Bent cable",[](auto& doc){auto resistor=doc.addPart(PartKind::Resistor,430,200);doc.connect({"board","GPIO14"},{resistor,"1"});const_cast<CircuitWire&>(doc.wires().back()).points={{300,500}};});
        canvas->fitCircuit();QTest::qWait(50);for(auto* item:canvas->scene()->items())if(item->data(0)=="wire")item->setSelected(true);
        QTest::qWait(30);QGraphicsItem* handle=nullptr;for(auto* item:canvas->scene()->items())if(item->data(0)=="bend")handle=item;QVERIFY(handle);
        const auto from=canvas->mapFromScene(handle->scenePos());const auto to=canvas->mapFromScene(QPointF(350,450));
        QTest::mousePress(canvas->viewport(),Qt::LeftButton,Qt::NoModifier,from);QTest::mouseMove(canvas->viewport(),to,30);QTest::mouseRelease(canvas->viewport(),Qt::LeftButton,Qt::NoModifier,to);
        QTRY_VERIFY(window.circuit().wires().front().points.front().x!=300);const auto moved=window.circuit().wires().front().points.front();QVERIFY(std::abs(moved.x-350)<=10&&std::abs(moved.y-450)<=10);
        canvas->history()->undo();QCOMPARE(window.circuit().wires().front().points.front().x,300.0);canvas->setEditingEnabled(false);for(auto* item:canvas->scene()->items())QVERIFY(item->data(0)!="bend");canvas->setEditingEnabled(true);
    }
    void darkModeAndMenuContrast(){Theme::apply(true);MainWindow window;window.show();QTest::qWait(60);QCOMPARE(qApp->palette().color(QPalette::Base),QColor("#17212f"));auto* menu=window.menuBar()->actions().front()->menu();menu->popup(window.mapToGlobal(QPoint(20,40)));QTest::qWait(50);const auto image=menu->grab().toImage();const auto rect=menu->actionGeometry(menu->actions().at(3));int white=0;for(int y=rect.top();y<=rect.bottom();++y)for(int x=rect.left()+10;x<=rect.right()-10;++x){auto c=image.pixelColor(x,y);if(c.red()>210&&c.green()>210&&c.blue()>210)++white;}QVERIFY(white>20);QVERIFY(image.pixelColor(3,3).lightness()<60);menu->close();Theme::apply(false);QCOMPARE(qApp->palette().color(QPalette::Base),QColor("#ffffff"));}
    void tutorialOpens(){MainWindow window;window.show();bool shown=false;QTimer::singleShot(100,&window,[&]{auto* dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());if(dialog){auto* browser=dialog->findChild<QTextBrowser*>();shown=browser&&browser->toPlainText().contains("setCursor");dialog->reject();}});bool triggered=false;for(auto* action:window.findChildren<QAction*>())if(action->text().startsWith("Tutorial:")){triggered=true;action->trigger();break;}QVERIFY(triggered&&shown);}
    void legacyProjectFormat(){QTemporaryDir dir;SavedProject project;project.sketch.order={"sketch.ino"};project.sketch.files["sketch.ino"]="void setup(){} void loop(){delay(20);}";const auto old=dir.filePath("legacy.thinkercad");ProjectIO::save(old,project);QFile file(old);QVERIFY(file.open(QIODevice::ReadOnly));auto object=QJsonDocument::fromJson(file.readAll()).object();file.close();object["format"]="ThinkerCad";QVERIFY(write(old,QString::fromUtf8(QJsonDocument(object).toJson())));MainWindow window;QVERIFY(window.openPath(old));QVERIFY(window.saveProjectTo(dir.filePath("new.thinkcad")));QVERIFY(window.openPath(dir.filePath("new.thinkcad")));}
    void rejectUnsafeProjectPaths(){QTemporaryDir dir;SavedProject p;p.sketch.order={"sketch.ino"};p.sketch.files["sketch.ino"]="void setup(){} void loop(){}";const auto path=dir.filePath("safe.thinkercad");ProjectIO::save(path,p);QFile file(path);QVERIFY(file.open(QIODevice::ReadOnly));auto json=QJsonDocument::fromJson(file.readAll()).object();file.close();auto files=json["files"].toArray();files.append(QJsonObject{{"name","../escaped.cpp"},{"text",""}});json["files"]=files;QVERIFY(write(path,QString::fromUtf8(QJsonDocument(json).toJson())));QVERIFY_EXCEPTION_THROWN(ProjectIO::load(path),std::runtime_error);QVERIFY(!QFileInfo::exists(dir.filePath("../escaped.cpp")));}
};
QTEST_MAIN(DesktopTests)
#include "desktop_tests.moc"
