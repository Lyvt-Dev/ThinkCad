#include "simulationinspector.hpp"
#include "circuitcanvas.hpp"
#include "modulecontrols.hpp"
#include "language.hpp"
#include <QTabWidget>
#include <QComboBox>
#include <QTreeWidget>
#include <QTableWidget>
#include <QHeaderView>
#include <QLabel>
#include <QCheckBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSignalBlocker>
#include <QShowEvent>
#include <QHideEvent>
#include <QFileDialog>
#include <QSaveFile>
#include <QMessageBox>
using namespace thinkercad;
namespace {
QString text(const char* de,const char* en){return Language::text(QString::fromUtf8(Language::current()=="de"?de:en));}
QString levelText(Level level){switch(level){case Level::Low:return "LOW";case Level::High:return "HIGH";case Level::Contention:return text("Treiberkonflikt","Contention");default:return text("Unbestimmt / offen","Undefined / floating");}}
QString volts(const std::optional<double>& value){return value?QString::number(*value,'f',4)+" V":QStringLiteral("—");}
QString hex(const std::vector<std::uint8_t>& bytes,std::size_t total){QByteArray data;for(auto byte:bytes)data+=char(byte);auto result=QString::fromLatin1(data.toHex(' '));if(total>bytes.size())result+=" … ("+QString::number(total)+" B)";return result;}
QWidget* page(QTabWidget* tabs,const QString& label,QVBoxLayout*& layout){auto* widget=new QWidget;layout=new QVBoxLayout(widget);tabs->addTab(widget,label);return widget;}
}
SimulationInspector::SimulationInspector(CircuitCanvas* canvas,const CircuitState& state,const ModuleDevices& devices,const UartBus& uart,BusTraffic& traffic,BoardLookup board,QWidget* parent):QWidget(parent),canvas_(canvas),state_(&state),devices_(&devices),uart_(&uart),traffic_(&traffic),board_(std::move(board)){
    setObjectName("simulationInspector");auto* layout=new QVBoxLayout(this);auto* tabs=new QTabWidget;layout->addWidget(tabs);QVBoxLayout* body;
    page(tabs,text("Pins & Netze","Pins & nets"),body);parts_=new QComboBox;parts_->setObjectName("inspectorPart");body->addWidget(parts_);actuatorTelemetry_=new QLabel;actuatorTelemetry_->setObjectName("actuatorTelemetry");actuatorTelemetry_->setTextFormat(Qt::PlainText);actuatorTelemetry_->setWordWrap(true);body->addWidget(actuatorTelemetry_);pins_=new QTreeWidget;pins_->setObjectName("inspectorPins");pins_->setHeaderLabels({text("Pin","Pin"),text("Spannung","Voltage"),text("Logik","Logic"),text("Modus / Ausgang","Mode / output")});body->addWidget(pins_);net_=new QLabel;net_->setObjectName("inspectorNet");net_->setTextFormat(Qt::PlainText);net_->setWordWrap(true);net_->setTextInteractionFlags(Qt::TextSelectableByMouse);body->addWidget(net_);historyView_=new QTreeWidget;historyView_->setObjectName("inspectorPinHistory");historyView_->setHeaderLabels({text("Änderung bei µs","Change at µs"),text("Spannung","Voltage"),text("Logik","Logic")});body->addWidget(historyView_);
    page(tabs,text("Geräteregister","Device registers"),body);modules_=new QComboBox;modules_->setObjectName("inspectorRegisterDevice");body->addWidget(modules_);registerHint_=new QLabel(text("Modellierte Register; Lesen verändert den Gerätezustand nicht.","Modeled registers; inspecting them does not change device state."));registerHint_->setWordWrap(true);body->addWidget(registerHint_);registers_=new QTableWidget(16,16);registers_->setObjectName("inspectorRegisters");registers_->setEditTriggers(QAbstractItemView::NoEditTriggers);QStringList columns,rows;for(int n=0;n<16;++n){columns<<QString::number(n,16).toUpper();rows<<QString("0x%1").arg(n*16,2,16,QChar('0')).toUpper();}registers_->setHorizontalHeaderLabels(columns);registers_->setVerticalHeaderLabels(rows);registers_->horizontalHeader()->setDefaultSectionSize(40);body->addWidget(registers_);
    page(tabs,text("Bus-Verkehr","Bus traffic"),body);auto* captureRow=new QHBoxLayout;body->addLayout(captureRow);capture_=new QCheckBox(text("Aufzeichnen, solange geöffnet","Capture while open"));capture_->setObjectName("captureBusTraffic");capture_->setChecked(true);captureRow->addWidget(capture_);busFilter_=new QComboBox;busFilter_->addItems({text("Alle Busse","All buses"),"I2C","SPI","UART"});captureRow->addWidget(busFilter_);auto* clear=new QPushButton(text("Leeren","Clear"));captureRow->addWidget(clear);auto* save=new QPushButton(text("CSV speichern…","Save CSV…"));captureRow->addWidget(save);trafficHint_=new QLabel;body->addWidget(trafficHint_);busView_=new QTreeWidget;busView_->setObjectName("inspectorBusTraffic");busView_->setHeaderLabels({"µs",text("Board","Board"),text("Bus","Bus"),text("Ziel","Target"),"Hz / baud",text("TX (hex)","TX (hex)"),text("RX (hex)","RX (hex)"),text("Status","Status")});body->addWidget(busView_);
    page(tabs,text("UART-Zustand","UART state"),body);uartView_=new QTreeWidget;uartView_->setObjectName("inspectorUartState");uartView_->setHeaderLabels({text("Board / Port","Board / port"),"RX","TX","Baud",text("Format","Format"),text("Framing","Framing"),text("Überlauf","Overrun"),text("Kollision","Collision")});body->addWidget(uartView_);
    page(tabs,text("Umgebung / Sensoren","Environment / sensors"),body);environment_=new QComboBox;environment_->setObjectName("environmentDevice");body->addWidget(environment_);auto* hint=new QLabel(text("Sensorwerte auch während der Simulation ändern. Adresse und Geometrie bleiben gesperrt.","Change sensor values while running. Address and geometry stay locked."));hint->setWordWrap(true);body->addWidget(hint);controls_=new QWidget;controls_->setLayout(new QVBoxLayout);controls_->layout()->setContentsMargins(0,0,0,0);body->addWidget(controls_);body->addStretch();
    connect(parts_,qOverload<int>(&QComboBox::currentIndexChanged),this,[this]{pins_->clear();selected_={};history_.clear();historyDirty_=true;refreshPins();});connect(pins_,&QTreeWidget::currentItemChanged,this,[this]{selectPin();});connect(modules_,qOverload<int>(&QComboBox::currentIndexChanged),this,[this]{registerPart_.clear();refreshRegisters();});connect(environment_,qOverload<int>(&QComboBox::currentIndexChanged),this,[this]{refreshEnvironment();});connect(capture_,&QCheckBox::toggled,this,[this]{if(traffic_)traffic_->setConsumer("inspector",capturing());});connect(busFilter_,qOverload<int>(&QComboBox::currentIndexChanged),this,[this]{trafficRevision_=~std::uint64_t(0);refreshTraffic();});connect(clear,&QPushButton::clicked,this,[this]{if(traffic_){traffic_->clear();refreshTraffic();}});
    connect(save,&QPushButton::clicked,this,[this]{if(!traffic_)return;const auto path=QFileDialog::getSaveFileName(this,text("Bus-Verkehr speichern","Save bus traffic"),"bus-traffic.csv","CSV (*.csv)");if(path.isEmpty())return;auto quote=[](QString value){value.replace('"',"\"\"");return '"'+value+'"';};QByteArray data="time_us,board,bus,target,hz,tx_hex,rx_hex,status,stop,repeated_start\n";for(const auto& entry:traffic_->entries())data+=(QString::number(entry.time)+','+quote(QString::fromStdString(entry.board))+','+quote(QString::fromStdString(entry.bus))+','+quote(QString::fromStdString(entry.target))+','+QString::number(entry.clock)+','+quote(hex(entry.tx,entry.txBytes))+','+quote(hex(entry.rx,entry.rxBytes))+','+quote(QString::fromStdString(entry.status))+','+QString::number(entry.stop)+','+QString::number(entry.repeatedStart)+'\n').toUtf8();QSaveFile file(path);if(!file.open(QIODevice::WriteOnly)||file.write(data)!=data.size()||!file.commit())QMessageBox::warning(this,text("Bus-Verkehr speichern","Save bus traffic"),file.errorString());});
    timer_.setInterval(200);connect(&timer_,&QTimer::timeout,this,&SimulationInspector::refresh);configure();
}
bool SimulationInspector::capturing()const{return traffic_&&isVisible()&&capture_->isChecked();}
void SimulationInspector::showEvent(QShowEvent* event){QWidget::showEvent(event);if(!traffic_)return;traffic_->setConsumer("inspector",capturing());timer_.start();refresh();}
void SimulationInspector::hideEvent(QHideEvent* event){timer_.stop();if(traffic_)traffic_->setConsumer("inspector",false);QWidget::hideEvent(event);}
void SimulationInspector::shutdown(){timer_.stop();if(traffic_)traffic_->setConsumer("inspector",false);traffic_=nullptr;state_=nullptr;devices_=nullptr;uart_=nullptr;canvas_=nullptr;board_={};}
void SimulationInspector::reset(){now_=0;history_.clear();historyDirty_=true;if(traffic_)traffic_->clear();registerPart_.clear();lastRegisters_.reset();}
void SimulationInspector::configure(){
    if(!canvas_)return;const auto& doc=canvas_->document();QByteArray signature;for(const auto& part:doc.parts())signature+=QByteArray::fromStdString(part.id)+'\0'+QByteArray::fromStdString(part.model)+'\0'+QByteArray::number(int(part.kind))+'\n';if(signature==configured_)return;configured_=signature;const auto part=parts_->currentData(),module=modules_->currentData(),environment=environment_->currentData();QSignalBlocker p(parts_),m(modules_),e(environment_);parts_->clear();modules_->clear();environment_->clear();
    for(const auto& item:doc.parts()){const auto id=QString::fromStdString(item.id);parts_->addItem(id+" · "+Language::text(QString::fromStdString(componentSpec(item.kind).label)),id);if(item.kind==PartKind::Module){const auto& model=ModuleRegistry::get(item.model);if(model.bus=="i2c"||model.bus=="spi")modules_->addItem(id+" · "+QString::fromStdString(model.label),id);environment_->addItem(id+" · "+QString::fromStdString(model.label),id);}}
    auto restore=[](QComboBox* box,const QVariant& id){const auto index=box->findData(id);box->setCurrentIndex(index>=0?index:box->count()?0:-1);};restore(parts_,part);restore(modules_,module);restore(environment_,environment);pins_->clear();selected_={};history_.clear();historyDirty_=true;netSignature_.clear();registerPart_.clear();
}
void SimulationInspector::showPins(const QString &part) {
    configure();
    if (auto *tabs = findChild<QTabWidget *>()) tabs->setCurrentIndex(0);
    const int index = parts_->findData(part);
    if (index >= 0) parts_->setCurrentIndex(index);
    refreshPins();
}
void SimulationInspector::selectPin(){auto* item=pins_->currentItem();if(!item)return;const Endpoint next{parts_->currentData().toString().toStdString(),item->text(0).toStdString()};if(next==selected_)return;selected_=next;history_.clear();historyDirty_=true;netSignature_.clear();if(state_)sample(now_,*state_);}
void SimulationInspector::sample(Microseconds time,const CircuitState& state){
    now_=time;if(!isVisible()||selected_.part.empty())return;const auto found=state.voltage.find(selected_);if(found==state.voltage.end())return;const auto value=found->second;const auto level=state.level(selected_);
    if(!history_.empty()){const auto& last=history_.back();if(last.voltage==value&&last.level==level)return;if(time<last.time)history_.clear();}
    if(history_.size()>=2048)history_.pop_front();history_.push_back({time,value,level});historyDirty_=true;
}
void SimulationInspector::refreshPins(){
    if(!canvas_||!state_)return;const auto id=parts_->currentData().toString().toStdString();const auto& doc=canvas_->document();const auto found=std::find_if(doc.parts().begin(),doc.parts().end(),[&](const auto& part){return part.id==id;});if(found==doc.parts().end()){pins_->clear();return;}const auto names=CircuitDocument::pins(*found);const auto* board=board_?board_(id):nullptr;
    QString actuatorText;
    if(found->kind==PartKind::Module&&(found->model.starts_with("stepper-")||found->model=="a4988")){
        auto value=[&](const std::string& suffix)->QString{auto it=state_->partValues.find(id+suffix);return it==state_->partValues.end()?QString::fromUtf8("—"):QString::number(it->second,'g',8);};
        actuatorText=text("Position / Winkel","Position / angle")+": "+value("")+"\n"+
            text("Schritte / Mikroschritte / Spulen","Steps / microsteps / coils")+": "+value("/steps")+" / "+value("/microsteps")+" / "+value("/coils")+"\n"+
            text("Sketch-Ziel (Schritte)","Sketch target (steps)")+": "+value("/target")+"\n"+
            text("Schrittrate (Vollschritte/s)","Step rate (full steps/s)")+": "+value("/stepRate")+" · rpm: "+value("/rpm")+"\n"+
            text("Zielposition wird vom Sketch verwaltet. Keine Last- oder Resonanzsimulation.","The sketch manages the target position. Load and resonance are not simulated.");
    }else if(found->kind==PartKind::Module&&found->model=="relay-5v"){
        const bool on=state_->partValues.contains(id)&&state_->partValues.at(id)>0;
        actuatorText=on?text("Relais angezogen: COM mit NO verbunden.","Relay energized: COM connected to NO."):text("Relais abgefallen: COM mit NC verbunden.","Relay released: COM connected to NC.");
    }
    actuatorTelemetry_->setText(actuatorText);actuatorTelemetry_->setVisible(!actuatorText.isEmpty());
    if(pins_->topLevelItemCount()!=int(names.size())){QSignalBlocker block(pins_);pins_->clear();for(const auto& pin:names)new QTreeWidgetItem(pins_,{QString::fromStdString(pin)});pins_->setCurrentItem(pins_->topLevelItem(0));selectPin();}
    for(int i=0;i<int(names.size());++i){auto* item=pins_->topLevelItem(i);const Endpoint endpoint{id,names[i]};const auto voltage=state_->voltage.find(endpoint);item->setText(1,volts(voltage==state_->voltage.end()?std::optional<double>{}:voltage->second));item->setText(2,levelText(state_->level(endpoint)));QString mode;if(board){const int gpio=board->profile().pinNumber(names[i]);if(gpio>=0&&std::find(board->profile().gpio.begin(),board->profile().gpio.end(),gpio)!=board->profile().gpio.end()){const auto& pin=board->pin(gpio);mode=pin.mode==PinMode::Output?text("Ausgang","Output"):pin.mode==PinMode::InputPullup?"INPUT_PULLUP":pin.mode==PinMode::InputPulldown?"INPUT_PULLDOWN":text("Eingang","Input");if(pin.mode==PinMode::Output)mode+=pin.pwm||pin.frequency?QString(" · %1 Hz · %2 %").arg(pin.frequency).arg(pin.duty*100,0,'f',1):pin.output?" · HIGH":" · LOW";}}item->setText(3,mode);}
    if(!selected_.part.empty()){
        QByteArray signature=QByteArray::fromStdString(selected_.part+"/"+selected_.pin);for(const auto& part:doc.parts())signature+=QByteArray::fromStdString(part.id+":"+part.netName)+',';for(const auto& wire:doc.wires())signature+=QByteArray::fromStdString(wire.id+wire.a.part+wire.a.pin+wire.b.part+wire.b.pin)+',';
        if(signature!=netSignature_){netSignature_=signature;QStringList connected;for(const auto& part:doc.parts())for(const auto& pin:CircuitDocument::pins(part))if(doc.connected(selected_,{part.id,pin}))connected.append(QString::fromStdString(part.id+":"+pin));net_->setText(text("Verbunden: ","Connected: ")+connected.join(", "));}
    }
    if(historyDirty_){historyDirty_=false;historyView_->clear();int start=std::max(0,int(history_.size())-128);for(int i=start;i<int(history_.size());++i){const auto& entry=history_[i];new QTreeWidgetItem(historyView_,{QString::number(entry.time),volts(entry.voltage),levelText(entry.level)});}}
}
void SimulationInspector::refreshRegisters(){
    if(!canvas_||!devices_)return;const auto id=modules_->currentData().toString().toStdString();const auto values=devices_->peekRegisters(id);if(id==registerPart_&&values==lastRegisters_)return;registerPart_=id;lastRegisters_=values;
    for(int i=0;i<256;++i){auto* item=registers_->item(i/16,i%16);if(!item){item=new QTableWidgetItem;registers_->setItem(i/16,i%16,item);}item->setText(values?QString("%1").arg((*values)[i],2,16,QChar('0')).toUpper():QStringLiteral("—"));}
}
void SimulationInspector::refreshTraffic(){
    if(!traffic_||trafficRevision_==traffic_->revision())return;trafficRevision_=traffic_->revision();busView_->clear();const auto filter=busFilter_->currentIndex()?busFilter_->currentText():QString();
    for(const auto& entry:traffic_->entries()){if(!filter.isEmpty()&&filter!=QString::fromStdString(entry.bus))continue;auto status=QString::fromStdString(entry.status);if(entry.bus=="I2C")status+=entry.repeatedStart?" · RESTART":" · START";if(entry.bus=="I2C")status+=entry.stop?" · STOP":" · no STOP";new QTreeWidgetItem(busView_,{QString::number(entry.time),QString::fromStdString(entry.board),QString::fromStdString(entry.bus),QString::fromStdString(entry.target),QString::number(entry.clock),hex(entry.tx,entry.txBytes),hex(entry.rx,entry.rxBytes),status});}
    trafficHint_->setText(text("Begrenzter Verlauf: ","Bounded history: ")+QString::number(traffic_->entries().size())+" / "+QString::number(traffic_->capacity())+text(" · überschrieben: "," · overwritten: ")+QString::number(traffic_->overwritten()));
}
void SimulationInspector::refreshEnvironment(){
    if(!canvas_)return;const auto id=environment_->currentData().toString().toStdString();if(id.empty()){delete environmentControls_;environmentControls_=nullptr;return;}const auto& part=canvas_->document().part(id);
    if(!environmentControls_||environmentControls_->partId()!=id||environmentControls_->modelId()!=part.model){delete environmentControls_;environmentControls_=new ModuleControls(canvas_,id,controls_);controls_->layout()->addWidget(environmentControls_);}environmentControls_->refresh();
}
void SimulationInspector::refresh(){
    if(!canvas_||!state_||!isVisible())return;if(language_!=Language::current()){language_=Language::current();netSignature_.clear();historyDirty_=true;trafficRevision_=~std::uint64_t(0);for(int i=0;i<parts_->count();++i){const auto id=parts_->itemData(i).toString().toStdString();const auto& parts=canvas_->document().parts();const auto found=std::find_if(parts.begin(),parts.end(),[&](const auto& part){return part.id==id;});if(found!=parts.end())parts_->setItemText(i,QString::fromStdString(id)+" · "+Language::text(QString::fromStdString(componentSpec(found->kind).label)));}}configure();refreshPins();refreshRegisters();refreshTraffic();refreshEnvironment();uartView_->clear();if(uart_)for(const auto& part:canvas_->document().parts())if(part.kind==PartKind::Esp8266)for(int port:UartBus::supportedPorts(BoardProfile::get(part.model)))if(uart_->active(part.id,port)){const auto c=uart_->config(part.id,port);const auto e=uart_->errors(part.id,port);new QTreeWidgetItem(uartView_,{QString::fromStdString(part.id)+":"+QString::number(port),QString::number(c.rx),QString::number(c.tx),QString::number(c.baud),QString::number(c.dataBits)+(c.parity==UartBus::Parity::Odd?"O":c.parity==UartBus::Parity::Even?"E":"N")+QString::number(c.stopBits),QString::number(e.framing),QString::number(e.overrun),QString::number(e.collision)});}
}
