#include "audiocodecdecoder.hpp"
#include "codeeditor.hpp"
#include "thinkercad/modules.hpp"
#include "componentoptions.hpp"
#include <QColorDialog>
#include <QPainter>
#include <QAction>
#include <QImage>
#include <QCheckBox>
#include <stdexcept>
#include "peripheralhub.hpp"
#include "examples.hpp"
#include "mainwindow.hpp"
#include "serialtools.hpp"
#include "componentdesign.hpp"
#include <QtTest>
#include <QApplication>
#include <QDir>
#include <QDockWidget>
#include <QSettings>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSlider>
#include <QTabWidget>
using namespace thinkercad;
class PeripheralTests:public QObject {
    Q_OBJECT
    QTemporaryDir temporary_;
    static std::string find(const CircuitDocument& d,PartKind kind){for(const auto& p:d.parts())if(p.kind==kind)return p.id;throw std::runtime_error("Missing test part");}
    static std::vector<int> request(PeripheralHub& hub,const std::string& id,Esp8266Board& board,const CircuitDocument& doc,const CircuitState& state,Microseconds now,const QByteArray& command){std::vector<int> result;if(!hub.handle(id,board,doc,state,now,command,[&](int n){result.push_back(n);}))throw std::runtime_error("Unhandled command");return result;}
    void shot(MainWindow& window,const QString& name){auto root=qEnvironmentVariable("THINKCAD_REVIEW_DIR");if(root.isEmpty())return;QDir().mkpath(root);QCoreApplication::processEvents();window.resize(1500,1000);window.findChild<QDockWidget*>("codeDock")->show();auto* source=window.findChild<QComboBox*>("serialSource");source->setCurrentIndex(source->findData("board"));QTest::qWait(80);window.canvas()->fitCircuit();QTest::qWait(100);QVERIFY(window.grab().save(root+"/"+name+".png"));}
private slots:
    void initTestCase(){QCoreApplication::setOrganizationName("ThinkCadPeripheralTests");QSettings::setDefaultFormat(QSettings::IniFormat);QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,temporary_.path());qputenv("THINKCAD_RECOVERY_DIR",temporary_.filePath("recovery").toUtf8());ComponentDesign::set("blocks");}
    void cleanup(){ComponentDesign::set("blocks");}
    void rawI2cRegisterModels(){
        auto project=ExampleCatalog::create("lcd_i2c");auto& doc=project.circuit;Esp8266Board board;board.configure("arduino-uno");I2cDevices bus;
        auto transfer=[&](int address,const std::vector<uint8_t>& bytes,int count=-1){return bus.transfer(doc,doc.solve(board),"board",18,19,address,bytes,count);};
        QCOMPARE(transfer(0x26,{}).status,2);QCOMPARE(transfer(0x27,{}).status,0);
        const auto id=find(doc,PartKind::I2cCharacterLcd);
        auto nibble=[&](int n,bool rs=false){uint8_t v=uint8_t((n<<4)|8|rs);QCOMPARE(transfer(0x27,{v,uint8_t(v|4),v}).status,0);};
        auto byte=[&](int value,bool rs=false){nibble(value>>4,rs);nibble(value&15,rs);};
        nibble(3);nibble(3);nibble(3);nibble(2);byte(0x28);byte(0x0c);byte(0x06);byte(0x01);byte('H',true);
        auto frame=bus.frame(doc.part(id));const int glyph[]={0x7f,8,8,8,0x7f};for(int x=0;x<5;++x)for(int y=0;y<8;++y)QCOMPARE(frame.pixel(x,y),bool((glyph[x]>>y)&1));
        byte(0x40);for(int row=0;row<8;++row)byte(31,true);byte(0x94);byte(0,true);frame=bus.frame(doc.part(id));QVERIFY(frame.pixel(0,20)&&frame.pixel(4,27));
        byte(0x01);frame=bus.frame(doc.part(id));QVERIFY(std::none_of(frame.pixels.begin(),frame.pixels.end(),[](auto p){return p!=0;}));
        transfer(0x27,{8});QCOMPARE(transfer(0x27,{},1).bytes,std::vector<uint8_t>{8});
        auto duplicate=doc.addPart(PartKind::I2cCharacterLcd,900,0);for(const auto& pin:{"VCC","GND","SDA","SCL"})doc.connect({duplicate,pin},{id,pin});QCOMPARE(transfer(0x27,{}).status,4);doc.removePart(duplicate);
        const auto wire=std::find_if(doc.wires().begin(),doc.wires().end(),[&](auto& w){return w.a==Endpoint{id,"VCC"}||w.b==Endpoint{id,"VCC"};})->id;doc.removeWire(wire);QCOMPARE(transfer(0x27,{}).status,2);
        auto oled=doc.addPart(PartKind::Oled,700,0);doc.connect({oled,"VCC"},{"board","5V"});doc.connect({oled,"GND"},{"board","GND"});doc.connect({oled,"SDA"},{"board","A4"});doc.connect({oled,"SCL"},{"board","A5"});
        QCOMPARE(transfer(0x3c,{0,0xa1,0xc8,0xaf,0x20,0,0x21,0,127,0x22,0,7}).status,0);transfer(0x3c,{0x40,1,2});frame=bus.frame(doc.part(oled));QVERIFY(frame.pixel(0,0)&&frame.pixel(1,1)&&!frame.pixel(0,1));
        transfer(0x3c,{0,0xa7});QVERIFY(!bus.frame(doc.part(oled)).pixel(0,0));transfer(0x3c,{0,0xae});frame=bus.frame(doc.part(oled));QVERIFY(std::none_of(frame.pixels.begin(),frame.pixels.end(),[](auto p){return p!=0;}));
        QCOMPARE(transfer(0x3c,{},4).bytes.size(),std::size_t(0));
    }
    void wireSketchCompilesAndDrivesLcd(){
        MainWindow window;window.show();QVERIFY(window.loadExample("lcd_i2c"));
        auto* editor=qobject_cast<QPlainTextEdit*>(window.findChild<QTabWidget*>("codeTabs")->currentWidget());QVERIFY(editor);
        editor->setPlainText(R"sketch(
#include <Wire.h>
void sendNibble(byte n,byte rs=0){byte v=(n<<4)|8|rs;Wire.beginTransmission(0x27);Wire.write(v);Wire.write(byte(v|4));Wire.write(v);Wire.endTransmission();}
void sendByte(byte n,byte rs=0){sendNibble(n>>4,rs);sendNibble(n&15,rs);}
void setup(){Serial.begin(9600);Wire.begin();
Wire.beginTransmission(0x27);Wire.write("hello world");Serial.println(Wire.endTransmission());
Wire.beginTransmission(0x26);Serial.println(Wire.endTransmission());
Wire.beginTransmission(0x27);Wire.write(byte(8));Wire.endTransmission(false);
Wire.requestFrom(0x27,1);Serial.println(Wire.read());
sendNibble(3);sendNibble(3);sendNibble(3);sendNibble(2);sendByte(0x28);sendByte(0x0c);sendByte(6);sendByte(1);sendByte('H',1);Serial.println("Wire ready");}
void loop(){delay(20);}
)sketch");editor->document()->setModified(true);window.startSimulation();
        auto* serial=window.findChild<SerialTools*>();QTRY_VERIFY_WITH_TIMEOUT(serial->monitorText().contains("Wire ready")||!window.simulationActive(),15000);
        QVERIFY2(serial->monitorText().contains("Wire ready"),qPrintable(window.findChild<QPlainTextEdit*>("consoleOutput")->toPlainText()));
        QVERIFY2(serial->monitorText().remove('\r').contains("0\n2\n8"),qPrintable(serial->monitorText()));const auto& frame=window.displayFrame(find(window.circuit(),PartKind::I2cCharacterLcd));QVERIFY(frame.pixel(0,0)&&frame.pixel(4,6)&&!frame.pixel(1,0));window.stopSimulation();
    }
    void i2cLcdSupplyAndWiringChecks(){
        auto project=ExampleCatalog::create("lcd_i2c");auto& doc=project.circuit;Esp8266Board board;board.configure("arduino-uno");I2cDevices bus;
        const auto lcd=find(doc,PartKind::I2cCharacterLcd);
        std::vector<std::string> wires;for(const auto& w:doc.wires())if(w.a==Endpoint{lcd,"VCC"}||w.b==Endpoint{lcd,"VCC"})wires.push_back(w.id);for(const auto& id:wires)doc.removeWire(id);
        const auto supply=doc.addPart(PartKind::PowerSupply,0,500);doc.connect({supply,"-"},{"board","GND"});doc.connect({supply,"+"},{lcd,"VCC"});
        auto transfer=[&](int sda=18,int scl=19,int address=0x27){return bus.transfer(doc,doc.solve(board),"board",sda,scl,address,{}).status;};
        for(double volts:{2.7,3.3,5.0}){doc.part(supply).value=volts;QCOMPARE(transfer(),0);}
        for(double volts:{2.6,5.6}){doc.part(supply).value=volts;QCOMPARE(transfer(),2);}
        doc.part(supply).value=3.3;QCOMPARE(transfer(19,18),2);QCOMPARE(transfer(18,19,0x26),2);
        doc.removePart(supply);QCOMPARE(transfer(),2);
    }
    void liquidCrystalI2cCustomPins20x4_data(){QTest::addColumn<bool>("emulation");QTest::newRow("simulation")<<false;QTest::newRow("emulation")<<true;}
    void liquidCrystalI2cCustomPins20x4(){
        QFETCH(bool,emulation);MainWindow window;window.show();QVERIFY(window.loadExample("lcd_i2c"));
        const auto lcd=find(window.circuit(),PartKind::I2cCharacterLcd);
        window.canvas()->mutate("custom S3 LCD",[&](auto& doc){
            doc.part("board").model="esp32-s3";doc.part(lcd).model="lcd-20x4";
            std::vector<std::string> wires;for(const auto& w:doc.wires())wires.push_back(w.id);for(const auto& id:wires)doc.removeWire(id);
            doc.connect({"board","GND"},{lcd,"GND"});doc.connect({"board","3V3"},{lcd,"VCC"});
            doc.connect({"board","GPIO8"},{lcd,"SDA"});doc.connect({"board","GPIO9"},{lcd,"SCL"});
        });
        auto* editor=qobject_cast<QPlainTextEdit*>(window.findChild<QTabWidget*>("codeTabs")->currentWidget());QVERIFY(editor);
        editor->setPlainText(R"sketch(
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
const uint8_t LCD_ADDRESS=0x27;
const int SW=19;
const int STATUS=20;
LiquidCrystal_I2C lcd(LCD_ADDRESS,20,4);
void setup(){pinMode(SW,INPUT_PULLUP);pinMode(STATUS,OUTPUT);Wire.begin(8,9);lcd.init();lcd.begin(20,4);lcd.backlight();lcd.setCursor(0,0);lcd.print("Hello World");}
void loop(){if(digitalRead(SW)==HIGH){digitalWrite(STATUS,HIGH);}else{digitalWrite(STATUS,LOW);}delay(100);}
)sketch");editor->document()->setModified(true);QVERIFY(window.setExecutionMode(emulation?ExecutionMode::Emulation:ExecutionMode::Simulation));window.startSimulation();
        QTRY_VERIFY_WITH_TIMEOUT(window.displayFrame(lcd).pixel(0,0)||!window.simulationActive(),15000);
        QVERIFY2(window.simulationActive(),qPrintable(window.findChild<QPlainTextEdit*>("consoleOutput")->toPlainText()));
        QVERIFY(window.displayFrame(lcd).pixel(0,0));QVERIFY(window.displayFrame(lcd).pixel(64,0));QTRY_VERIFY_WITH_TIMEOUT(window.simulationTime()>300000,5000);QVERIFY(window.displayFrame(lcd).pixel(0,0));QCOMPARE(window.circuitState().level({"board","GPIO20"}),Level::High);shot(window,emulation?"lcd-s3-3v3-emulation":"lcd-s3-3v3-simulation");window.stopSimulation();
    }
    void expandedComponentArtworkGallery(){
        const auto root=qEnvironmentVariable("THINKCAD_REVIEW_DIR");if(root.isEmpty())QSKIP("Set THINKCAD_REVIEW_DIR to export the component review gallery");
        QDir().mkpath(root);ComponentDesign::set("realistic");int index=0,page=0;QImage image;
        for(const auto& [id,model]:ModuleRegistry::definitions()){
            if(index%12==0){if(!image.isNull())QVERIFY(image.save(root+QString("/component-gallery-%1.png").arg(page++,2,10,QChar('0'))));image=QImage(1800,1200,QImage::Format_ARGB32_Premultiplied);image.fill(QColor("#f0f1ee"));}
            CircuitPart part;part.id="preview";part.kind=PartKind::Module;part.model=id;part.color="#dd3e46";
            QPainter painter(&image);int card=index%12;QRectF area((card%4)*450,(card/4)*400,450,400);
            painter.setPen(QColor("#263632"));painter.setFont(QFont("Sans Serif",12));painter.drawText(area.adjusted(12,10,-12,-345),Qt::AlignCenter|Qt::TextWordWrap,QString::fromStdString(model.label));
            const auto bounds=ComponentDesign::bounds(part);double scale=std::min(1.,std::min(400./bounds.width(),290./bounds.height()));
            painter.translate(area.center().x()-bounds.center().x()*scale,area.top()+220-bounds.center().y()*scale);painter.scale(scale,scale);
            ComponentDesign::paint(painter,part,{}, {},1,false);painter.setPen(QPen(QColor("#8b9691"),1));painter.setBrush(QColor("#d2d7d3"));for(const auto& pin:CircuitDocument::pins(part))painter.drawEllipse(ComponentDesign::pinPosition(part,pin),2,2);
            painter.end();++index;
        }
        if(!image.isNull())QVERIFY(image.save(root+QString("/component-gallery-%1.png").arg(page,2,10,QChar('0'))));QVERIFY(index>80);
    }
    void attinyBatteryWiringWithDiagnostics(){
        MainWindow window;window.show();auto* diagnostics=window.findChild<QDockWidget*>("diagnosticsDock");diagnostics->show();diagnostics->raise();QTest::qWait(5);
        auto* canvas=window.canvas();std::string battery;
        canvas->mutate("Bare ATtiny",[&](auto& d){d.part("board").model="attiny85";battery=d.addPart(PartKind::PowerSupply,300,0);d.part(battery).value=5;});
        for(int repeat=0;repeat<5;++repeat){canvas->connectPin({battery,"+"});canvas->connectPin({"board","VCC"});QCoreApplication::processEvents();canvas->connectPin({battery,"-"});canvas->connectPin({"board","GND"});QCoreApplication::processEvents();QVERIFY(window.circuitState().errors.empty());canvas->history()->undo();canvas->history()->undo();QCoreApplication::processEvents();}
    }
    void staleModuleControlsAndPendingAnalogEdits(){
        MainWindow window;window.show();auto* canvas=window.canvas();std::string module;
        canvas->mutate("Joystick",[&](auto& d){module=d.addPart(PartKind::Module,400,0);d.part(module).model="joystick";});canvas->selectPart(module);
        auto* input=window.findChild<QDoubleSpinBox*>("moduleParam_x");QVERIFY(input);canvas->mutate("Remove selected module",[&](auto& d){d.removePart(module);});
        input->setValue(12);QCoreApplication::processEvents();QVERIFY(!canvas->document().parts().empty());
        for(int repeat=0;repeat<20;++repeat){std::string analog;canvas->mutate("Analog source",[&](auto& d){analog=d.addPart(PartKind::AnalogSource,400,250);});canvas->selectPart(analog);auto* editor=window.findChild<QDoubleSpinBox*>("componentValue");editor->show();editor->setFocus();editor->findChild<QLineEdit*>()->setText("3.25");canvas->mutate("Delete with a pending edit",[&](auto& d){d.removePart(analog);});QCoreApplication::processEvents();canvas->history()->undo();QCoreApplication::processEvents();canvas->history()->redo();QCoreApplication::processEvents();}
    }
    void directJoystickAndMomentaryControls(){
        MainWindow window;window.show();ComponentDesign::set("realistic");auto* canvas=window.canvas();std::string joystick,button;
        canvas->mutate("Direct controls",[&](auto& d){joystick=d.addPart(PartKind::Module,400,0);d.part(joystick).model="joystick";button=d.addPart(PartKind::Module,620,0);d.part(button).model="arcade-button";});
        canvas->fitCircuit();canvas->setEditingEnabled(false);
        auto point=[&](const std::string& id,QPointF mm){const auto& part=canvas->document().part(id);return canvas->mapFromScene(QPointF(part.x,part.y)+mm*6);};
        const auto center=point(joystick,{13,14}),right=point(joystick,{18,14});
        QTest::mousePress(canvas->viewport(),Qt::LeftButton,Qt::NoModifier,center);
        QTest::mouseMove(canvas->viewport(),right,30);
        QVERIFY(parameter(canvas->document().part(joystick),"x",50)>95);
        QVERIFY(std::abs(parameter(canvas->document().part(joystick),"y",50)-50)<3);
        QTest::mouseRelease(canvas->viewport(),Qt::LeftButton,Qt::NoModifier,right);
        QCOMPARE(parameter(canvas->document().part(joystick),"x",0),50.);QCOMPARE(parameter(canvas->document().part(joystick),"y",0),50.);
        QTest::mousePress(canvas->viewport(),Qt::RightButton,Qt::NoModifier,center);QCOMPARE(parameter(canvas->document().part(joystick),"pressed",0),1.);
        QTest::mouseRelease(canvas->viewport(),Qt::RightButton,Qt::NoModifier,center);QCOMPARE(parameter(canvas->document().part(joystick),"pressed",1),0.);
        const auto buttonCenter=point(button,{15,15});QTest::mousePress(canvas->viewport(),Qt::LeftButton,Qt::NoModifier,buttonCenter);QCOMPARE(parameter(canvas->document().part(button),"pressed",0),1.);
        QTest::mouseRelease(canvas->viewport(),Qt::LeftButton,Qt::NoModifier,buttonCenter);QCOMPARE(parameter(canvas->document().part(button),"pressed",1),0.);
        shot(window,"joystick-direct-controls");canvas->setEditingEnabled(true);
    }
    void liveSensorSlidersSurviveDeletion(){
        MainWindow window;window.show();auto* canvas=window.canvas();std::string sensor;
        canvas->mutate("Light sensor",[&](auto& d){sensor=d.addPart(PartKind::Photoresistor,400,0);});
        canvas->showRuntimeControls(sensor);auto* slider=window.findChild<QSlider*>("live_value");QVERIFY(slider);slider->setValue(750);QVERIFY(canvas->document().part(sensor).value>0);
        QPointer<QSlider> pending=slider;canvas->mutate("Remove live sensor",[&](auto& d){d.removePart(sensor);});if(pending)pending->setValue(250);QCoreApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);QVERIFY(pending.isNull());
    }
    void servoVerbatimEndpointSweep_data(){QTest::addColumn<bool>("emulation");QTest::addColumn<bool>("realistic");for(bool emulation:{false,true})for(bool realistic:{false,true})QTest::newRow(qPrintable(QString(emulation?"emulation":"simulation")+(realistic?"-realistic":"-blocks")))<<emulation<<realistic;}
    void servoVerbatimEndpointSweep(){
        QFETCH(bool,emulation);QFETCH(bool,realistic);MainWindow window;window.show();QVERIFY(window.loadExample("servo_sweep"));const auto servo=find(window.circuit(),PartKind::ServoMotor);
        window.canvas()->mutate("Exact D5 servo wiring",[&](auto& d){d.clearWires();d.connect({servo,"VCC"},{"board","5V"});d.connect({servo,"GND"},{"board","GND"});d.connect({servo,"SIGNAL"},{"board","D5"});});
        auto* editor=window.findChild<CodeEditor*>();QVERIFY(editor);editor->setPlainText("#include <Servo.h>\nServo myservo;\nvoid setup(){myservo.attach(5);}\nvoid loop(){myservo.write(0);delay(1000);myservo.write(180);delay(1000);}\n");editor->document()->setModified(true);
        ComponentDesign::set(realistic?"realistic":"blocks");window.canvas()->rebuild();QVERIFY(window.setExecutionMode(emulation?ExecutionMode::Emulation:ExecutionMode::Simulation));window.startSimulation();
        auto angle=[&]{auto it=window.circuitState().partValues.find(servo);return it==window.circuitState().partValues.end()?-999.:it->second;};
        QTRY_VERIFY_WITH_TIMEOUT((angle()>=0&&angle()<2)||!window.simulationActive(),15000);QVERIFY2(window.simulationActive(),qPrintable(window.findChild<QPlainTextEdit*>("consoleOutput")->toPlainText()));
        QVERIFY(angle()<2);int intermediate=0;QTRY_VERIFY_WITH_TIMEOUT(([&]{if(angle()>5&&angle()<175)++intermediate;return angle()>178||!window.simulationActive();})(),6000);QVERIFY2(intermediate>=3,"The running sketch must show intermediate servo positions, not snap between endpoints");QVERIFY2(window.simulationActive(),qPrintable(window.findChild<QPlainTextEdit*>("consoleOutput")->toPlainText()));QVERIFY(angle()>178);
        QTRY_VERIFY_WITH_TIMEOUT(angle()<2||!window.simulationActive(),6000);QVERIFY2(window.simulationActive(),qPrintable(window.findChild<QPlainTextEdit*>("consoleOutput")->toPlainText()));QVERIFY(angle()<2);window.stopSimulation();
    }
    void servoEndpointsAreVisuallyDifferent(){
        CircuitPart servo;servo.id="servo";servo.kind=PartKind::ServoMotor;ComponentDesign::set("realistic");
        auto image=[&](double angle){QImage result(240,240,QImage::Format_ARGB32);result.fill(Qt::white);QPainter painter(&result);painter.translate(40,40);CircuitState state;state.partValues[servo.id]=angle;ComponentDesign::paint(painter,servo,state,{},1,false);return result;};
        auto zero=image(0),end=image(180);int changed=0;for(int y=0;y<zero.height();++y)for(int x=0;x<zero.width();++x)if(zero.pixel(x,y)!=end.pixel(x,y))++changed;QVERIFY2(changed>200,"Servo endpoints must have visibly different arm positions");
    }
    void codecMp3DecodesToRealPcm(){
        QFile fixture(QFINDTESTDATA("fixtures/vs1053-tone.mp3"));QVERIFY(fixture.open(QIODevice::ReadOnly));auto encoded=fixture.readAll();AudioCodecDecoder decoder;std::shared_ptr<const std::vector<float>> pcm;unsigned rate=0,callbacks=0;std::size_t delivered=0;QString error;
        decoder.notice=[&](const auto& text){error=text;};decoder.decoded=[&](const auto& id,auto data,unsigned hz,std::uint64_t generation){QCOMPARE(id,std::string("codec"));QCOMPARE(generation,std::uint64_t(42));++callbacks;delivered+=data->size();pcm=std::move(data);rate=hz;};decoder.append("codec",{encoded.begin(),encoded.end()},42);
        QTRY_VERIFY_WITH_TIMEOUT(pcm||!error.isEmpty(),10000);QVERIFY2(pcm,qPrintable(error));QVERIFY(rate>0);QVERIFY(pcm->size()>4000);std::vector<int> crossings;bool armed=false;for(std::size_t i=(pcm->size()/4)&~std::size_t(1);i<pcm->size()*3/4;i+=2){if((*pcm)[i]<-.002)armed=true;if(armed&&(*pcm)[i]>.002){crossings.push_back(int(i/2));armed=false;}}QVERIFY(crossings.size()>10);double frequency=double(crossings.size()-1)*rate/(crossings.back()-crossings.front());QVERIFY(std::abs(frequency-440)<10);auto first=pcm->size();decoder.append("codec",{encoded.begin(),encoded.end()},42);QTRY_VERIFY_WITH_TIMEOUT(callbacks==2||!error.isEmpty(),10000);QVERIFY2(callbacks==2,qPrintable(error));QVERIFY(delivered>first*1.7);QVERIFY(delivered<first*2.5);
    }
    void newComponentPropertiesAndFootprints(){
        MainWindow window;window.show();QVERIFY(window.loadExample("lcd_i2c"));std::string led,bread;window.canvas()->mutate("parts",[&](auto& d){led=d.addPart(PartKind::Led,600,400);bread=d.addPart(PartKind::Breadboard30,500,300);});
        auto* inspector=window.findChild<QDockWidget*>("inspectorDock");inspector->show();inspector->raise();window.canvas()->selectPart(led);auto* options=window.findChild<QWidget*>("componentOptions");QVERIFY(inspector->isAncestorOf(options));auto* color=options->findChild<QPushButton*>("componentColor");QVERIFY(color);QTimer::singleShot(0,[]{auto* dialog=qobject_cast<QColorDialog*>(QApplication::activeModalWidget());QVERIFY(dialog);dialog->setCurrentColor(QColor("#287ec8"));dialog->accept();});color->click();QCOMPARE(window.circuit().part(led).color,std::string("#287ec8"));
        window.canvas()->selectPart(bread);auto* count=options->findChild<QComboBox*>("breadboardContacts");auto* rails=options->findChild<QCheckBox*>("breadboardPowerRails");count->setCurrentIndex(count->findData(63));rails->setChecked(true);QCOMPARE(CircuitDocument::pins(window.circuit().part(bread)).size(),std::size_t(830));window.canvas()->history()->undo();QVERIFY(!breadboardRails(window.circuit().part(bread)));window.canvas()->history()->redo();QVERIFY(breadboardRails(window.circuit().part(bread)));window.canvas()->mutate("Arrange review",[&](auto& d){for(const auto& p:d.parts()){auto& part=d.part(p.id);if(p.kind==PartKind::Esp8266){part.x=-400;part.y=0;}if(p.kind==PartKind::I2cCharacterLcd){part.x=200;part.y=0;}}d.part(bread).x=-300;d.part(bread).y=420;d.part(led).x=-380;d.part(led).y=420;});window.canvas()->selectPart(bread);ComponentDesign::set("realistic");window.canvas()->rebuild();shot(window,"new-breadboard-led-properties");
        for(const auto& [id,definition]:ModuleRegistry::definitions()){CircuitPart part{"sample",PartKind::Module,0,0,0,false,"#287ec8",id};const auto bounds=ComponentDesign::bounds(part);for(const auto& pin:CircuitDocument::pins(part))QVERIFY2(bounds.contains(ComponentDesign::pinPosition(part,pin)),(id+"."+pin).c_str());}
    }
    void serialStreamSketchParsesMonitorInput(){
        MainWindow window;window.show();QVERIFY(window.loadExample("lcd_i2c"));
        auto* editor=qobject_cast<QPlainTextEdit*>(window.findChild<QTabWidget*>("codeTabs")->currentWidget());QVERIFY(editor);
        editor->setPlainText(R"sketch(
void setup(){Serial.begin(115200);Serial.setTimeout(50);Serial.println("INPUT_READY");}
void loop(){if(Serial.available()){
long number=Serial.parseInt();Serial.read();float decimal=Serial.parseFloat();Serial.read();
String text=Serial.readStringUntil('\n');Serial.print("Parsed:");Serial.print(number);Serial.print(',');Serial.print(decimal,1);Serial.print(',');Serial.println(text);
}delay(10);}
)sketch");editor->document()->setModified(true);window.startSimulation();auto* serial=window.findChild<SerialTools*>();
        QTRY_VERIFY_WITH_TIMEOUT(serial->monitorText().contains("INPUT_READY")||!window.simulationActive(),15000);
        QVERIFY2(window.simulationActive(),qPrintable(window.findChild<QPlainTextEdit*>("consoleOutput")->toPlainText()));
        auto* source=serial->findChild<QComboBox*>("serialSource");source->setCurrentIndex(source->findData("board"));auto* input=serial->findChild<QLineEdit*>("serialInput");input->setText("42,3.5,hello");QTest::keyClick(input,Qt::Key_Return);
        QTRY_VERIFY_WITH_TIMEOUT(serial->monitorText().contains("Parsed:42,3.5,hello"),5000);QVERIFY(window.simulationActive());window.stopSimulation();
    }
    void irRemoteAndLed(){auto project=ExampleCatalog::create("ir_loopback");auto& d=project.circuit;Esp8266Board board;board.configure("esp32");PeripheralHub hub;auto call=[&](const QByteArray& text){return request(hub,"board",board,d,d.solve(board),0,text);};QCOMPARE(call("IR_RX_BEGIN 1 4"),std::vector<int>{1});QCOMPARE(call("IR_TX_BEGIN 2 2"),std::vector<int>{1});board.pinMode(2,PinMode::Output);board.digitalWrite(2,true);QCOMPARE(call("IR_SEND 2 0 69"),std::vector<int>{1});QCOMPARE(call("IR_DECODE 1"),(std::vector<int>{1,0,69,0}));QCOMPARE(call("IR_DECODE 1"),std::vector<int>{0});call("IR_RESUME 1");QCOMPARE(call("IR_DECODE 1"),std::vector<int>{0});auto remote=d.addPart(PartKind::InfraredRemote,600,0);hub.inputs(d);d.part(remote).pressed=true;hub.inputs(d);QCOMPARE(call("IR_DECODE 1"),(std::vector<int>{1,0,69,0}));call("IR_RESUME 1");hub.inputs(d);QCOMPARE(call("IR_DECODE 1"),std::vector<int>{0});d.part(remote).pressed=false;hub.inputs(d);d.part(remote).parameters["channel"]=1;d.part(remote).pressed=true;hub.inputs(d);QCOMPARE(call("IR_DECODE 1"),std::vector<int>{0});auto receiver=find(d,PartKind::InfraredReceiver);auto wire=std::find_if(d.wires().begin(),d.wires().end(),[&](const auto& w){return w.a==Endpoint{receiver,"VCC"}||w.b==Endpoint{receiver,"VCC"};})->id;d.removeWire(wire);QVERIFY_EXCEPTION_THROWN(call("IR_DECODE 1"),std::runtime_error);}
    void irMissingSeriesResistor(){auto p=ExampleCatalog::create("ir_loopback");auto& d=p.circuit;auto r=find(d,PartKind::Resistor);d.removePart(r);auto led=find(d,PartKind::InfraredLed);d.connect({"board","GPIO2"},{led,"A"});Esp8266Board board;board.configure("esp32");PeripheralHub hub;QCOMPARE(request(hub,"board",board,d,d.solve(board),0,"IR_TX_BEGIN 1 2"),std::vector<int>{0});}
    void nfcPresenceUidAndNdef(){auto p=ExampleCatalog::create("nfc_reader");auto& d=p.circuit;Esp8266Board board;board.configure("esp32");board.pinMode(26,PinMode::Output);board.digitalWrite(26,true);PeripheralHub hub;auto call=[&](const QByteArray& text){return request(hub,"board",board,d,d.solve(board),0,text);};QCOMPARE(call("NFC_BEGIN 1 21 22 27 26"),std::vector<int>{1});QCOMPARE(call("NFC_POLL 1"),(std::vector<int>{7,4,161,178,195,212,229,246}));auto page=call("NFC_PAGE 1 4");QCOMPARE(page.front(),4);QCOMPARE(page[1],3);QByteArray memory;for(int i=4;i<40;++i){auto values=call("NFC_PAGE 1 "+QByteArray::number(i));for(int j=1;j<5;++j)memory.append(char(values[j]));}QVERIFY(memory.contains("ThinkCad NFC test card"));auto card=find(d,PartKind::NfcTestCard);d.part(card).pressed=false;QCOMPARE(call("NFC_PAGE 1 4"),std::vector<int>{0});QCOMPARE(call("NFC_POLL 1"),std::vector<int>{0});d.part(card).pressed=true;d.part(card).x=5000;QCOMPARE(call("NFC_POLL 1"),std::vector<int>{0});QVERIFY_EXCEPTION_THROWN(call("NFC_PAGE 1 40"),std::runtime_error);auto restored=ProjectIO::deserialize(ProjectIO::serialize(p));QCOMPARE(restored.circuit.parts().size(),d.parts().size());}
    void nrfPacketsAcknowledgementsAndMismatch(){auto p=ExampleCatalog::create("nrf24_pair");auto& d=p.circuit;std::string second;for(const auto& part:d.parts())if(part.kind==PartKind::Esp8266&&part.id!="board")second=part.id;Esp8266Board a,b;a.configure("esp32");b.configure("esp32");std::map<std::string,const Esp8266Board*> boards{{"board",&a},{second,&b}};PeripheralHub hub;Microseconds now=0;auto call=[&](bool rx,const QByteArray& text){return request(hub,rx?second:"board",rx?b:a,d,d.solve(boards),now,text);};QCOMPARE(call(false,"RF_BEGIN 1 18 19 23 5 16"),std::vector<int>{1});QCOMPARE(call(true,"RF_BEGIN 1 18 19 23 5 16"),std::vector<int>{1});call(false,"RF_DEST 1 5443414431");call(true,"RF_PIPE 1 1 5443414431");call(true,"RF_LISTEN 1 1");QCOMPARE(call(false,"RF_SEND 1 68656c6c6f"),std::vector<int>{1});QCOMPARE(call(true,"RF_AVAILABLE 1"),std::vector<int>{0});now=1000;QCOMPARE(call(true,"RF_AVAILABLE 1"),(std::vector<int>{5,1}));QCOMPARE(call(true,"RF_READ 1"),(std::vector<int>{5,104,101,108,108,111}));call(true,"RF_CONFIG 1 77 1000 2 1");QCOMPARE(call(false,"RF_SEND 1 01"),std::vector<int>{0});call(true,"RF_CONFIG 1 76 250 2 1");QCOMPARE(call(false,"RF_SEND 1 01"),std::vector<int>{0});call(true,"RF_CONFIG 1 76 1000 1 1");QCOMPARE(call(false,"RF_SEND 1 01"),std::vector<int>{0});call(true,"RF_CONFIG 1 76 1000 2 1");call(true,"RF_PIPE 1 1 5443414432");QCOMPARE(call(false,"RF_SEND 1 01"),std::vector<int>{0});call(true,"RF_PIPE 1 1 5443414431");call(true,"RF_LISTEN 1 0");QCOMPARE(call(false,"RF_SEND 1 01"),std::vector<int>{0});call(true,"RF_LISTEN 1 1");for(int i=0;i<3;++i)QCOMPARE(call(false,"RF_SEND 1 01"),std::vector<int>{1});QCOMPARE(call(false,"RF_SEND 1 01"),std::vector<int>{0});now+=1000;for(int i=0;i<3;++i)call(true,"RF_READ 1");auto radio=find(d,PartKind::RadioTransceiver);for(const auto& part:d.parts())if(part.kind==PartKind::RadioTransceiver&&part.id!=radio){radio=part.id;break;}auto wire=std::find_if(d.wires().begin(),d.wires().end(),[&](const auto& w){return w.a==Endpoint{radio,"VCC"}||w.b==Endpoint{radio,"VCC"};})->id;d.removeWire(wire);QCOMPARE(call(false,"RF_SEND 1 01"),std::vector<int>{0});}
    void askFrequencyMismatch(){auto p=ExampleCatalog::create("ask_pair");auto& d=p.circuit;Esp8266Board board;board.configure("esp32");PeripheralHub hub;Microseconds now=0;auto call=[&](const QByteArray& text){return request(hub,"board",board,d,d.solve(board),now,text);};QCOMPARE(call("ASK_TX_BEGIN 1 25"),std::vector<int>{1});QCOMPARE(call("ASK_RX_BEGIN 2 26"),std::vector<int>{1});call("ASK_SEND 1 16 1 a55a");QCOMPARE(call("ASK_READ 2"),std::vector<int>{0});now=10000;QCOMPARE(call("ASK_READ 2"),(std::vector<int>{16,0xa55a,0}));d.part(find(d,PartKind::RadioReceiver)).value=315;call("ASK_SEND 1 16 1 a55a");now+=10000;QCOMPARE(call("ASK_READ 2"),std::vector<int>{0});}
    void nrfRejectsWrongPinsAndFiveVoltSupply(){auto p=ExampleCatalog::create("nrf24_pair");auto& d=p.circuit;Esp8266Board board;board.configure("esp32");PeripheralHub hub;auto call=[&](const QByteArray& text){return request(hub,"board",board,d,d.solve(board),0,text);};QCOMPARE(call("RF_BEGIN 1 18 19 23 5 25"),std::vector<int>{0});auto radio=find(d,PartKind::RadioTransceiver);auto wire=std::find_if(d.wires().begin(),d.wires().end(),[&](const auto& w){return w.a==Endpoint{radio,"VCC"}||w.b==Endpoint{radio,"VCC"};})->id;d.removeWire(wire);auto supply=d.addPart(PartKind::PowerSupply,0,500);d.connect({supply,"-"},{"board","GND"});d.connect({supply,"+"},{radio,"VCC"});QCOMPARE(call("RF_BEGIN 1 18 19 23 5 16"),std::vector<int>{0});}
    void editableNfcDataPersists(){MainWindow window;window.show();QVERIFY(window.loadExample("nfc_reader"));auto card=find(window.circuit(),PartKind::NfcTestCard);window.canvas()->selectPart(card);auto* uid=window.findChild<QLineEdit*>("nfcUid");auto* text=window.findChild<QLineEdit*>("nfcText");uid->setText("04:11:22:33:44:55:66");text->setText("ThinkCad custom tag");auto* properties=window.findChild<QWidget*>("wirelessProperties");auto* apply=properties->findChild<QPushButton*>();QVERIFY(apply);apply->click();QCOMPARE(window.circuit().part(card).parameters.at("uid1"),17.);QCOMPARE(window.circuit().part(card).model,std::string("ThinkCad custom tag"));auto path=temporary_.filePath("custom-nfc.thinkcad");QVERIFY(window.saveProjectTo(path));auto restored=ProjectIO::load(path);QCOMPARE(restored.circuit.part(card).parameters.at("uid6"),102.);QCOMPARE(restored.circuit.part(card).model,std::string("ThinkCad custom tag"));ComponentDesign::set("realistic");window.canvas()->rebuild();window.canvas()->selectPart(card);shot(window,"nfc-tag-properties");window.canvas()->history()->undo();QVERIFY(window.circuit().part(card).model.empty());}
    void completeSketches_data(){QTest::addColumn<QString>("example");QTest::addColumn<QString>("expected");QTest::newRow("TV remote")<<"ir_remote"<<"command=0x45";QTest::newRow("IR loopback")<<"ir_loopback"<<"IR loopback command=0x45";QTest::newRow("NFC")<<"nfc_reader"<<"NDEF TLV: 0x3";QTest::newRow("nRF24")<<"nrf24_pair"<<"RX: ThinkCad packet";QTest::newRow("ASK")<<"ask_pair"<<"ASK RX: 0xA55A";}
    void completeSketches(){QFETCH(QString,example);QFETCH(QString,expected);MainWindow window;window.show();window.resize(1500,1000);QVERIFY(window.loadExample(example));auto* serial=window.findChild<SerialTools*>();auto* dock=window.findChild<QDockWidget*>("serialDock");dock->show();window.startSimulation();if(example=="ir_remote"){QTRY_VERIFY_WITH_TIMEOUT(window.simulationTime()>10000,15000);window.canvas()->toggleButton(find(window.circuit(),PartKind::InfraredRemote));}QTRY_VERIFY_WITH_TIMEOUT(serial->monitorText().contains(expected)||!window.simulationActive(),15000);QVERIFY2(serial->monitorText().contains(expected),qPrintable(window.findChild<QPlainTextEdit*>("consoleOutput")->toPlainText()));QVERIFY(window.simulationActive());QVERIFY(window.circuitState().errors.empty());shot(window,example);if(example=="nfc_reader"){window.canvas()->toggleButton(find(window.circuit(),PartKind::NfcTestCard));QTRY_VERIFY_WITH_TIMEOUT(serial->monitorText().contains("No test card"),3000);}ComponentDesign::set("realistic");window.canvas()->rebuild();shot(window,example+"-flat");window.stopSimulation();}
};
QTEST_MAIN(PeripheralTests)
#include "peripheral_tests.moc"
