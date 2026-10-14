#include "mainwindow.hpp"
#include <QApplication>
#include <QDateTime>
#include <QFileInfo>
#include <QPainterPath>
#include <QTextCursor>
#include <cmath>
#include <QAction>
#include <QCheckBox>
#include <QCloseEvent>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QFileDialog>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QImage>
#include <QLabel>
#include <QListWidget>
#include <QMenuBar>
#include <QMessageBox>
#include <QPainter>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSaveFile>
#include <QSettings>
#include <QSlider>
#include <QStandardPaths>
#include <QStatusBar>
#include <QTextDocument>
#include <QTimer>
#include <QToolBar>
#include <QVBoxLayout>
#include <iostream>
using namespace thinkercad;
class CircuitItem final : public QGraphicsItem {
public:
    enum Kind { Board, Oled };
    Kind kind;
    const Esp8266Board* board;
    const OledDisplay* display;
    CircuitItem(Kind kind,const Esp8266Board* board,const OledDisplay* display) : kind(kind),board(board),display(display) {
        setFlags(ItemIsMovable | ItemIsSelectable | ItemSendsGeometryChanges);
        setCursor(Qt::OpenHandCursor);
    }
    QRectF boundingRect() const override {return kind==Board?QRectF(-4,-4,218,360):QRectF(-4,-4,300,210);}
    QVariant itemChange(GraphicsItemChange change,const QVariant& value) override {
        if(change==ItemPositionHasChanged && scene()) scene()->update();
        return QGraphicsItem::itemChange(change,value);
    }
    void paint(QPainter* p,const QStyleOptionGraphicsItem*,QWidget*) override {
        p->setRenderHint(QPainter::Antialiasing);
        p->setPen(QPen(isSelected()?QColor("#2563eb"):QColor("#263747"),isSelected()?3:1));
        p->setBrush(kind==Board?QColor("#183c3b"):QColor("#16395b"));
        p->drawRoundedRect(kind==Board?QRectF(0,0,210,346):QRectF(0,0,292,202),12,12);
        if(kind==Board) {
            p->setBrush(QColor("#c4cbd1"));p->setPen(Qt::NoPen);p->drawRoundedRect(40,28,130,90,5,5);
            p->setPen(QColor("#263747"));p->setFont(QFont("Segoe UI",13,QFont::Bold));p->drawText(QRectF(40,45,130,30),Qt::AlignCenter,"ESP8266");
            p->setFont(QFont("Segoe UI",9));p->drawText(QRectF(40,80,130,24),Qt::AlignCenter,"ESP-12E / 3.3 V");
            p->setBrush(QColor("#202b32"));p->drawRoundedRect(65,145,80,70,4,4);
            p->setPen(QColor("#cce0df"));p->drawText(QRectF(65,162,80,25),Qt::AlignCenter,"Wi-Fi MCU");
            const QStringList left{"3V3","GND","D0 · 16","D1 · 5","D2 · 4","D3 · 0","D4 · 2","A0"};
            const QStringList right{"VIN*","GND","D5 · 14","D6 · 12","D7 · 13","D8 · 15","RX · 3","TX · 1"};
            for(int i=0;i<8;++i) {
                p->setBrush(QColor("#d9cba7"));p->setPen(Qt::NoPen);p->drawEllipse(QPointF(12,137+i*23),4,4);p->drawEllipse(QPointF(198,137+i*23),4,4);
                p->setPen(QColor("#dbe9e7"));p->setFont(QFont("Segoe UI",8));p->drawText(QRectF(21,129+i*23,62,18),left[i]);p->drawText(QRectF(134,129+i*23,58,18),Qt::AlignRight,right[i]);
            }
            p->setPen(Qt::NoPen);p->setBrush(QColor("#acb6bf"));p->drawRoundedRect(84,321,42,25,2,2);
            const bool on=board->digitalRead(2)==Level::Low;
            p->setBrush(on?QColor("#36b5ff"):QColor("#123a52"));p->drawEllipse(QPointF(105,245),5,5);
            p->setPen(QColor("#d4e3e2"));p->drawText(QRectF(50,278,110,22),Qt::AlignCenter,"NodeMCU · ESP8266");
        } else {
            p->setBrush(QColor("#050c14"));p->setPen(QColor("#6a8293"));p->drawRect(QRectF(17,20,256,128));
            p->setPen(Qt::NoPen);p->setBrush(QColor("#9aebff"));
            const auto& f=display->frame();
            for(int y=0;y<64;++y)for(int x=0;x<128;++x)if(f.pixel(x,y))p->drawRect(QRectF(17+x*2,20+y*2,1.8,1.8));
            p->setPen(QColor("#d4e7fb"));p->setFont(QFont("Segoe UI",10));p->drawText(QRectF(15,156,260,22),Qt::AlignCenter,"SSD1306 · 128 × 64 · I²C 0x3C");
            p->setFont(QFont("Segoe UI",8));p->drawText(QRectF(15,181,260,18),Qt::AlignCenter,"GND        VCC        SCL        SDA");
        }
    }
};
class CircuitScene final : public QGraphicsScene {
public:
    CircuitItem *board=nullptr,*oled=nullptr;
    using QGraphicsScene::QGraphicsScene;
    void drawBackground(QPainter* p,const QRectF& r) override {
        p->fillRect(r,QColor("#f2f5f8"));p->setPen(QPen(QColor("#dbe1e8"),1));
        for(double x=std::floor(r.left()/20)*20;x<r.right();x+=20)
            for(double y=std::floor(r.top()/20)*20;y<r.bottom();y+=20)p->drawPoint(QPointF(x,y));
    }
    void drawForeground(QPainter* p,const QRectF&) override {
        if(!board || !oled) return;
        const QColor colors[]{QColor("#3a4553"),QColor("#d54a51"),QColor("#d6a128"),QColor("#368ac9")};
        const int boardRows[]{1,0,3,4};
        for(int i=0;i<4;++i) {
            auto a=board->mapToScene(QPointF(12,137+boardRows[i]*23));auto b=oled->mapToScene(QPointF(70+i*50,200));
            QPainterPath path(a);path.cubicTo(a+QPointF(-65,110+i*12),b+QPointF(0,85+i*12),b);
            p->setPen(QPen(colors[i],2.5));p->setBrush(Qt::NoBrush);p->drawPath(path);
        }
    }
};
MainWindow::MainWindow(bool smokeTest) : interpreter_(this),updater_(this),smokeTest_(smokeTest) {
    setWindowTitle("ThinkerCad — ESP8266 Studio");resize(1440,860);
    auto* toolbar=addToolBar("Werkzeuge");toolbar->setMovable(false);
    auto* open=toolbar->addAction("Sketch öffnen");auto* save=toolbar->addAction("Speichern");toolbar->addSeparator();
    run_=toolbar->addAction("▶ Simulation starten");auto* reset=toolbar->addAction("Zurücksetzen");
    auto* view=new QGraphicsView(this);auto* circuit=new CircuitScene(this);scene_=circuit;view->setScene(scene_);
    scene_->setSceneRect(-150,-70,1000,610);view->setRenderHint(QPainter::Antialiasing);view->setDragMode(QGraphicsView::RubberBandDrag);setCentralWidget(view);
    boardItem_=new CircuitItem(CircuitItem::Board,&board_,&oled_);oledItem_=new CircuitItem(CircuitItem::Oled,&board_,&oled_);
    scene_->addItem(boardItem_);scene_->addItem(oledItem_);boardItem_->setPos(25,25);oledItem_->setPos(380,55);circuit->board=boardItem_;circuit->oled=oledItem_;
    auto* zoomIn=toolbar->addAction("＋");auto* zoomOut=toolbar->addAction("−");connect(zoomIn,&QAction::triggered,this,[view]{if(view->transform().m11()<4)view->scale(1.2,1.2);});connect(zoomOut,&QAction::triggered,this,[view]{if(view->transform().m11()>.3)view->scale(1/1.2,1/1.2);});
    auto* codeDock=new QDockWidget("Arduino C++",this);editor_=new QPlainTextEdit(codeDock);editor_->setFont(QFont("Consolas",10));editor_->setTabStopDistance(28);codeDock->setWidget(editor_);codeDock->setMinimumWidth(420);addDockWidget(Qt::RightDockWidgetArea,codeDock);
    QFile example(QCoreApplication::applicationDirPath()+"/examples/animation.ino");
    if(example.open(QIODevice::ReadOnly))editor_->setPlainText(QString::fromUtf8(example.readAll()));
    else editor_->setPlainText("#include <Arduino.h>\nvoid setup() { pinMode(LED_BUILTIN, OUTPUT); }\nvoid loop() { digitalWrite(LED_BUILTIN, LOW); delay(500); digitalWrite(LED_BUILTIN, HIGH); delay(500); }\n");
    editor_->document()->setModified(false);
    auto* consoleDock=new QDockWidget("Compiler / Serial Monitor",this);console_=new QPlainTextEdit;console_->setReadOnly(true);console_->setMaximumBlockCount(1000);console_->setMinimumHeight(100);consoleDock->setWidget(console_);addDockWidget(Qt::BottomDockWidgetArea,consoleDock);
    auto* components=new QDockWidget("Bauteile und Eingänge",this);auto* panel=new QWidget;auto* layout=new QVBoxLayout(panel);
    auto* info=new QLabel("ESP8266 · 3,3 V\nSSD1306 OLED · 128 × 64\n\nVorverdrahteter Demo-Aufbau:\nSDA = D2 / GPIO4\nSCL = D1 / GPIO5\nVCC = 3V3, GND = GND\n\nBauteile im Arbeitsbereich\nmit der Maus verschieben.");info->setWordWrap(true);layout->addWidget(info);
    auto* button=new QCheckBox("Taster D6 / GPIO12 gedrückt");layout->addWidget(button);
    connect(button,&QCheckBox::toggled,this,[this](bool down){board_.driveInput(12,down?Level::Low:Level::Floating);refreshScene();});
    auto* adcLabel=new QLabel("A0: 0,000 V (roher ESP8266 ADC)");auto* adc=new QSlider(Qt::Horizontal);adc->setRange(0,1000);layout->addWidget(adcLabel);layout->addWidget(adc);
    connect(adc,&QSlider::valueChanged,this,[this,adcLabel](int v){board_.setAdcVoltage(v/1000.0);adcLabel->setText(QString("A0: %1 V · ADC %2").arg(v/1000.0,0,'f',3).arg(board_.analogRead()));});
    auto* libButton=new QPushButton("Bibliotheken verwalten…");layout->addWidget(libButton);layout->addStretch();components->setWidget(panel);addDockWidget(Qt::LeftDockWidgetArea,components);connect(libButton,&QPushButton::clicked,this,&MainWindow::libraries);
    auto* settings=menuBar()->addMenu("Einstellungen");auto* compilerAction=settings->addAction("Sketch-Compiler wählen…");
    connect(compilerAction,&QAction::triggered,this,[this]{auto path=QFileDialog::getOpenFileName(this,"g++ wählen (MinGW-w64 unter Windows)");if(!path.isEmpty())QSettings().setValue("compiler",path);});
    auto* imageAction=settings->addAction("Bild als Frame-Array einfügen…");
    connect(imageAction,&QAction::triggered,this,[this]{
        auto path=QFileDialog::getOpenFileName(this,"Monochromes Frame importieren",{},"Bilder (*.png *.bmp *.jpg)");if(path.isEmpty())return;
        QImage image(path);if(image.isNull()){appendLog("Bild konnte nicht gelesen werden.");return;}image=image.scaled(128,64,Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
        const auto name=QString("frame_%1").arg(QDateTime::currentMSecsSinceEpoch());QString code="\n#include <Arduino.h>\nconst uint8_t "+name+"[1024] PROGMEM = {\n";
        for(int y=0;y<64;++y){code+="    ";for(int bx=0;bx<16;++bx){unsigned b=0;for(int bit=0;bit<8;++bit){auto color=image.pixelColor(bx*8+bit,y);if(color.alpha()>127 && qGray(color.rgb())>=128)b|=0x80u>>bit;}code+=QString("0x%1,").arg(b,2,16,QChar('0'));}code+='\n';}
        code+="};\n// In loop(): display.drawBitmap(0, 0, "+name+", 128, 64, SSD1306_WHITE);\n";editor_->moveCursor(QTextCursor::Start);editor_->insertPlainText(code);appendLog("Frame-Array vor dem Sketch eingefügt. drawBitmap-Aufruf in loop() ergänzen.");
    });
    auto* help=menuBar()->addMenu("Hilfe");auto* update=help->addAction("Nach Updates suchen");connect(update,&QAction::triggered,&updater_,&Updater::check);
    auto* about=help->addAction("Über ThinkerCad");connect(about,&QAction::triggered,this,[this]{QMessageBox::information(this,"ThinkerCad " THINKERCAD_VERSION,"C++20 / Qt 6 · ESP8266 2D-Prototyp\n\nNative Sketch-Ausführung mit Arduino-API-Teilmenge. Keine Xtensa-CPU-Emulation. Nur vertrauenswürdigen Code und Bibliotheken ausführen.\n\nOLED-Frames, digitale Pegel und virtuelle Zeit sind implementiert. Freie Verdrahtung, PWM, Wi-Fi und vollständige Analogsimulation sind Ausbauziele.");});
    connect(open,&QAction::triggered,this,[this]{if(!maybeSave())return;auto path=QFileDialog::getOpenFileName(this,"Arduino-Sketch öffnen",{},"Arduino (*.ino);;C++ (*.cpp)");if(path.isEmpty())return;QFile file(path);if(file.open(QIODevice::ReadOnly)){editor_->setPlainText(QString::fromUtf8(file.readAll()));currentFile_=path;editor_->document()->setModified(false);}else appendLog("Sketch konnte nicht geöffnet werden.");});
    connect(save,&QAction::triggered,this,[this]{saveSketch();});connect(run_,&QAction::triggered,this,[this]{if(interpreter_.active())interpreter_.stop();else runSketch();});
    connect(reset,&QAction::triggered,this,[this,button,adc]{interpreter_.stop();simulator_.reset();board_.reset();oled_.reset();button->setChecked(false);adc->setValue(0);refreshScene();});
    connect(&interpreter_,&CodeInterpreter::log,this,&MainWindow::appendLog);connect(&interpreter_,&CodeInterpreter::protocol,this,&MainWindow::process);
    connect(&interpreter_,&CodeInterpreter::activeChanged,this,[this](bool active){run_->setText(active?"■ Simulation stoppen":"▶ Simulation starten");});
    connect(&updater_,&Updater::message,this,&MainWindow::appendLog);
    connect(&updater_,&Updater::updateReady,this,[this,help](const QString& version){appendLog("Version "+version+" ist geprüft und bereit. Zum Installieren neu starten.");auto* restart=help->addAction("Update installieren und neu starten");connect(restart,&QAction::triggered,this,[this]{if(!maybeSave())return;interpreter_.stop();if(updater_.restartAndInstall())QApplication::quit();});});
    status_=new QLabel;statusBar()->addWidget(status_);refreshScene();
    resizeDocks({consoleDock},{130},Qt::Vertical);
    QTimer::singleShot(0,this,[this,view]{view->fitInView(scene_->itemsBoundingRect().adjusted(-80,-35,35,100),Qt::KeepAspectRatio);});
    if(smokeTest_){QTimer::singleShot(0,this,&MainWindow::runSketch);QTimer::singleShot(20000,this,[]{std::cerr<<"Desktop smoke timed out\n";QCoreApplication::exit(1);});}
    else QTimer::singleShot(3000,&updater_,&Updater::check);
}
void MainWindow::appendLog(const QString& message){console_->appendPlainText(message);}
void MainWindow::runSketch(){
    simulator_.reset();oled_.reset();writes_=0;
    std::map<int,Level> external;
    for(int pin:Esp8266Board::nodeMcuGpio) external.emplace(pin,board_.pin(pin).external);
    external.emplace(1,board_.pin(1).external);external.emplace(3,board_.pin(3).external);
    const auto voltage=board_.adcVoltage();board_.reset();board_.setAdcVoltage(voltage);
    for(const auto& [pin,level]:external)board_.driveInput(pin,level);
    QSettings settings;const auto bundled=QCoreApplication::applicationDirPath()+"/toolchain/bin/g++.exe";
    const auto compiler=settings.value("compiler",QFileInfo::exists(bundled)?bundled:"g++").toString();
    interpreter_.start(editor_->toPlainText(),compiler,QCoreApplication::applicationDirPath()+"/sdk",settings.value("libraries").toStringList());refreshScene();
}
void MainWindow::process(const QByteArray& line){
    if(smokeDone_)return;
    try {
        const auto parts=line.split(' ');auto integer=[&](int index){if(index>=parts.size())throw std::invalid_argument("Missing protocol field");bool ok=false;int v=parts[index].toInt(&ok);if(!ok)throw std::invalid_argument("Invalid protocol integer");return v;};
        if(parts[0]=="TIME" && parts.size()==2){bool ok=false;const auto at=parts[1].toULongLong(&ok);if(!ok)throw std::invalid_argument("Invalid time");simulator_.runUntil(at);}
        else if(parts[0]=="MODE" && parts.size()==3){const auto mode=integer(2);if(mode<0||mode>2)throw std::invalid_argument("Invalid mode");board_.pinMode(integer(1),mode==1?PinMode::Output:mode==2?PinMode::InputPullup:PinMode::Input);}
        else if(parts[0]=="WRITE" && parts.size()==3){const auto value=integer(2);if(value!=0 && value!=1)throw std::invalid_argument("Invalid digital value");board_.digitalWrite(integer(1),value!=0);++writes_;}
        else if(parts[0]=="READ" && parts.size()==2){auto level=board_.digitalRead(integer(1));if(level==Level::Contention || level==Level::Floating)throw std::runtime_error("digitalRead: Pin schwebt oder hat widersprüchliche Treiber");interpreter_.answer(level==Level::High?1:0);}
        else if(parts[0]=="ADC" && parts.size()==1)interpreter_.answer(board_.analogRead());
        else if(parts[0]=="FRAME" && parts.size()==2){
            if(parts[1].size()!=2048)throw std::invalid_argument("Invalid frame size");for(char c:parts[1])if(!((c>='0'&&c<='9')||(c>='a'&&c<='f')))throw std::invalid_argument("Invalid frame bytes");
            const auto packed=QByteArray::fromHex(parts[1]);Frame frame;for(int y=0;y<64;++y)for(int x=0;x<128;++x)frame.setPixel(x,y,(static_cast<unsigned char>(packed[x+(y/8)*128])>>(y%8))&1);
            simulator_.schedule(simulator_.now(),[this,frame=std::move(frame)]{oled_.present(frame);});simulator_.runUntil(simulator_.now());
        }
        else if(parts[0]=="I2C" && parts.size()==3){if(integer(1)!=4 || integer(2)!=5)throw std::runtime_error("Demo-Verdrahtung erfordert SDA=GPIO4, SCL=GPIO5");}
        else if(parts[0]=="READY" && parts.size()==1)appendLog("setup() abgeschlossen.");
        else throw std::invalid_argument("Unbekannte Simulatornachricht");
        refreshScene();
        if(smokeTest_ && oled_.revision()>=2 && writes_>=1 && simulator_.now()>=100000){
            smokeDone_=true;
            const auto& f=oled_.frame();
            const bool correct=f.pixel(15,28) && !f.pixel(3,28) && f.pixel(8,52) && f.pixel(119,53) && simulator_.now()==123170;
            std::cout<<(correct?"Desktop smoke passed: compiled sketch, GPIO, I2C timing, moving OLED pixels\n":"Desktop smoke failed: unexpected pixels or timing\n");
            for(const auto& argument:QCoreApplication::arguments())
                if(argument.startsWith("--screenshot=")) grab().save(argument.mid(13));
            interpreter_.stop();QCoreApplication::exit(correct?0:1);
        }
    } catch(const std::exception& e){appendLog(QString("Simulation angehalten: ")+e.what());interpreter_.stop();if(smokeTest_)QCoreApplication::exit(1);}
}
void MainWindow::refreshScene(){scene_->update();status_->setText(QString("ESP8266 · %1 ms · OLED: %2 Frames").arg(simulator_.now()/1000).arg(oled_.revision()));}
void MainWindow::libraries(){
    QDialog dialog(this);dialog.setWindowTitle("Lokale Arduino/C++-Bibliotheken");dialog.resize(600,320);QVBoxLayout layout(&dialog);
    QLabel notice("Header und portable C++-Quellen werden mitkompiliert. ESP8266-Register, WLAN und nicht implementierte Arduino-APIs werden noch nicht unterstützt.");notice.setWordWrap(true);layout.addWidget(&notice);
    QListWidget list;list.addItems(QSettings().value("libraries").toStringList());layout.addWidget(&list);
    QPushButton add("Ordner hinzufügen"),remove("Aus Liste entfernen");layout.addWidget(&add);layout.addWidget(&remove);
    connect(&add,&QPushButton::clicked,&dialog,[&]{auto dir=QFileDialog::getExistingDirectory(&dialog,"Bibliotheksordner");if(!dir.isEmpty() && list.findItems(dir,Qt::MatchExactly).isEmpty())list.addItem(dir);});
    connect(&remove,&QPushButton::clicked,&dialog,[&]{delete list.takeItem(list.currentRow());});
    QDialogButtonBox buttons(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);layout.addWidget(&buttons);connect(&buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);connect(&buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    if(dialog.exec()==QDialog::Accepted){QStringList paths;for(int i=0;i<list.count();++i)paths<<list.item(i)->text();QSettings().setValue("libraries",paths);}
}
bool MainWindow::saveSketch(){auto path=currentFile_;if(path.isEmpty())path=QFileDialog::getSaveFileName(this,"Sketch speichern",{},"Arduino (*.ino)");if(path.isEmpty())return false;QSaveFile file(path);const auto bytes=editor_->toPlainText().toUtf8();if(!file.open(QIODevice::WriteOnly)||file.write(bytes)!=bytes.size()||!file.commit()){appendLog("Speichern fehlgeschlagen.");return false;}currentFile_=path;editor_->document()->setModified(false);return true;}
bool MainWindow::maybeSave(){if(!editor_->document()->isModified())return true;const auto answer=QMessageBox::question(this,"Ungespeicherter Sketch","Sketch vor dem Fortfahren speichern?",QMessageBox::Save|QMessageBox::Discard|QMessageBox::Cancel);if(answer==QMessageBox::Cancel)return false;return answer==QMessageBox::Discard||saveSketch();}
void MainWindow::closeEvent(QCloseEvent* event){if(maybeSave()){interpreter_.stop();event->accept();}else event->ignore();}
