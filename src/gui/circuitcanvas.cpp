#include "language.hpp"
#include "projectio.hpp"
#include <QApplication>
#include <QClipboard>
#include <QMimeData>
#include <QMenu>
#include <QSignalBlocker>
#include <QInputDialog>
#include "componentdesign.hpp"
#include <QStyleOptionGraphicsItem>
#include "circuitcanvas.hpp"
#include <QGraphicsEllipseItem>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QTimer>
#include <QScrollBar>
#include <QUndoCommand>
#include <QWheelEvent>
#include <cmath>
using namespace thinkercad;
namespace {
QPainterPath wirePath(QPointF a,QPointF b) {QPainterPath p(a);p.lineTo(b);return p;}
QString partName(PartKind kind){return Language::text(QString::fromStdString(componentSpec(kind).label));}
bool switchable(PartKind kind){return kind==PartKind::InfraredRemote||kind==PartKind::NfcTestCard||kind==PartKind::ToggleSwitch||kind==PartKind::Button||kind==PartKind::SlideSwitch||kind==PartKind::TiltSwitch||kind==PartKind::ReedSwitch;}
double blockBoardScale(const CircuitPart& part){return part.kind==PartKind::Esp8266&&BoardProfile::get(part.model).arduino()?1.25:1.0;}
QRectF blockBounds(const CircuitPart& part){const double scale=blockBoardScale(part);return part.kind==PartKind::CharacterLcd||part.kind==PartKind::I2cCharacterLcd?QRectF(-10,-10,250,160):part.kind==PartKind::Junction?QRectF(-12,-12,24,24):part.kind==PartKind::NetLabel?QRectF(-8,-26,150,42):part.kind==PartKind::Esp8266?QRectF(-10*scale,-10*scale,230*scale,scale*std::max(370,202+static_cast<int>((CircuitDocument::pins(part).size()+1)/2-1)*24)):part.kind==PartKind::Oled?QRectF(-10,-10,310,224):part.kind==PartKind::Breadboard30?QRectF(0,0,570,260):QRectF(-10,-10,170,std::max(120,64+static_cast<int>((CircuitDocument::pins(part).size()+1)/2)*20));}
QPointF localPin(const CircuitPart& part,const std::string& pin) {if(ComponentDesign::realistic())return ComponentDesign::pinPosition(part,pin);const auto kind=part.kind;
    if(kind==PartKind::CharacterLcd||kind==PartKind::I2cCharacterLcd){const auto names=CircuitDocument::pins(part);const auto i=std::find(names.begin(),names.end(),pin)-names.begin();return {10.+i*(kind==PartKind::CharacterLcd?14.:60.),140};}
    if(kind==PartKind::Junction||kind==PartKind::NetLabel)return {0,0};
    const auto names=CircuitDocument::pins(part);const int index=static_cast<int>(std::find(names.begin(),names.end(),pin)-names.begin());
    if(kind==PartKind::Esp8266){const int half=(names.size()+1)/2;return (index<half?QPointF(0,142+index*24):QPointF(210,142+(index-half)*24))*blockBoardScale(part);}
    if(kind==PartKind::Oled)return {47.0+index*66,200};
    if(kind==PartKind::Breadboard30){const int column=std::stoi(pin.substr(1))-1,row=pin[0]-'a';return {20.0+column*18,40.0+row*18+(row>=5?25:0)};}
    if(names.size()>2){const int half=(names.size()+1)/2;return {index<half?0.0:150.0,44.0+(index%half)*20};}
    return {index?150.0:0.0,64};
}
class WireItem final:public QGraphicsPathItem {
public:
    WireItem(const CircuitWire& wire,const QPainterPath& path):QGraphicsPathItem(path){setData(0,"wire");setData(1,QString::fromStdString(wire.id));setFlags(ItemIsSelectable);setZValue(2);setPen(QPen(QColor(QString::fromStdString(wire.color)),2.8,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));setToolTip(QString::fromStdString(wire.a.part+"."+wire.a.pin+" → "+wire.b.part+"."+wire.b.pin));}
    QPainterPath shape() const override{QPainterPathStroker s;s.setWidth(14);return s.createStroke(path());}
    void paint(QPainter* p,const QStyleOptionGraphicsItem*,QWidget*)override{auto color=pen();if(isSelected()){color.setColor(QColor("#2563eb"));color.setWidthF(5);}p->setPen(color);p->setBrush(Qt::NoBrush);p->drawPath(path());}
};
class BendHandle final:public QGraphicsEllipseItem {
    CircuitCanvas* canvas_;std::string id_;std::size_t index_;
public:
    BendHandle(CircuitCanvas* canvas,const std::string& id,std::size_t index,QPointF point):QGraphicsEllipseItem(-6,-6,12,12),canvas_(canvas),id_(id),index_(index){setData(0,"bend");setData(1,QString::fromStdString(id));setData(2,static_cast<int>(index));setPos(point);setBrush(Qt::white);setPen(QPen(QColor("#0d9488"),2));setFlags(ItemIsMovable|ItemIgnoresTransformations);setZValue(5);setCursor(Qt::SizeAllCursor);setToolTip(Language::text("Kabelknick ziehen"));}
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event)override{QGraphicsEllipseItem::mouseReleaseEvent(event);const auto point=pos();QTimer::singleShot(0,canvas_,[c=canvas_,id=id_,i=index_,point]{c->moveBend(id,i,point);});}
};
class Snapshot final:public QUndoCommand {
    CircuitCanvas* canvas_;CircuitDocument before_,after_;
public:
    Snapshot(CircuitCanvas* c,CircuitDocument before,CircuitDocument after,const QString& label):QUndoCommand(label),canvas_(c),before_(std::move(before)),after_(std::move(after)){}
    void undo()override{canvas_->applyDocument(before_);}void redo()override{canvas_->applyDocument(after_);}
};
class PinItem final:public QGraphicsEllipseItem {
    CircuitCanvas* canvas_;Endpoint endpoint_;
    bool realistic_=ComponentDesign::realistic();
public:
    PinItem(CircuitCanvas* c,Endpoint e,QGraphicsItem* parent):QGraphicsEllipseItem(ComponentDesign::realistic()?QRectF(-4,-4,8,8):QRectF(-7,-7,14,14),parent),canvas_(c),endpoint_(std::move(e)){setBrush(QColor("#ffffff"));setPen(QPen(QColor("#0d9488"),2));setZValue(3);setCursor(Qt::CrossCursor);setAcceptHoverEvents(true);const auto& part=canvas_->document().part(endpoint_.part);if(part.kind==PartKind::Junction){setRect(-4,-4,8,8);setBrush(QColor("#0d9488"));}QString description;
        const auto pin=QString::fromStdString(endpoint_.pin);
        if(endpoint_.pin=="A"&&(part.kind==PartKind::Led||part.kind==PartKind::Diode))description=Language::text("Anode (+)");
        else if(endpoint_.pin=="K")description=Language::text("Kathode (−)");
        else if(endpoint_.pin=="GND"||endpoint_.pin=="GND2")description=Language::text("Masse / Bezugspotential (0 V)");
        else if(endpoint_.pin=="SDA")description=Language::text("I²C-Datenleitung");
        else if(endpoint_.pin=="SCL")description=Language::text("I²C-Taktleitung");
        else if(endpoint_.pin=="VCC"||endpoint_.pin=="3V3"||endpoint_.pin=="3V3_2"||endpoint_.pin=="5V")description=Language::text("Versorgungsspannung");
        else if(endpoint_.pin=="W")description=Language::text("Schleifer");
        else if(endpoint_.pin=="COM")description=Language::text("Gemeinsame Kathode");
        else if(endpoint_.pin=="+"||endpoint_.pin=="-")description=Language::text(endpoint_.pin=="+"?"Pluspol":"Minuspol");
        else if(part.kind==PartKind::Esp8266)description=Language::text("GPIO / Boardanschluss");
        else description=Language::text("Anschluss")+" "+pin;
        setToolTip(pin+" — "+description+"\n"+QString::fromStdString(componentSpec(part.kind).label)+" · "+QString::fromStdString(part.id)+"\n"+Language::text("Zum Verbinden klicken"));}
    void paint(QPainter* painter,const QStyleOptionGraphicsItem* option,QWidget* widget)override{
        if(!realistic_){QGraphicsEllipseItem::paint(painter,option,widget);return;}
        const auto kind=canvas_->document().part(endpoint_.part).kind;
        const bool board=kind==PartKind::Esp8266||kind==PartKind::Oled;
        painter->setPen(QPen(board?QColor("#c7ad6d"):QColor("#929da4"),1.2));
        painter->setBrush(board?QColor("#c7ad6d"):QColor("#d4d9da"));painter->drawRoundedRect(QRectF(-4,-4,8,8),1,1);
        painter->setPen(Qt::NoPen);painter->setBrush(QColor("#202830"));painter->drawEllipse(QRectF(-2,-2,4,4));
        if(option->state&QStyle::State_MouseOver){painter->setPen(QPen(QColor("#14b8a6"),2));painter->setBrush(Qt::NoBrush);painter->drawEllipse(QRectF(-6,-6,12,12));}
    }
    const Endpoint& endpoint()const{return endpoint_;}
    void mousePressEvent(QGraphicsSceneMouseEvent* e)override{if(e->button()==Qt::LeftButton){canvas_->connectPin(endpoint_);e->accept();}else QGraphicsEllipseItem::mousePressEvent(e);}
};
}
class PartItem final:public QGraphicsItem {
    CircuitCanvas* canvas_;
    CircuitDocument before_;
    bool initializing_=true;
    bool realistic_=ComponentDesign::realistic();
    QRectF bounds_;
public:
    std::string id;PartKind kind;CircuitState state;Frame visible;double brightness=1;
    PartItem(CircuitCanvas* c,const CircuitPart& p):canvas_(c),bounds_(realistic_?ComponentDesign::bounds(p):blockBounds(p)),id(p.id),kind(p.kind){setData(0,"part");setData(1,QString::fromStdString(id));setFlags(ItemIsSelectable|ItemIsMovable|ItemSendsGeometryChanges);setZValue(1);setPos(p.x,p.y);setTransformOriginPoint(boundingRect().center());setRotation(p.rotation);initializing_=false;setCursor(Qt::OpenHandCursor);for(const auto& pin:CircuitDocument::pins(p)){auto* port=new PinItem(c,{id,pin},this);port->setPos(localPin(p,pin));}}
    QRectF boundingRect()const override{return bounds_;}
    QVariant itemChange(GraphicsItemChange change,const QVariant& v)override{
        if(change==ItemPositionChange&&!initializing_){auto pos=v.toPointF();pos=canvas_->snapped(pos);return QPointF(std::clamp(pos.x(),-9500.0,9500.0),std::clamp(pos.y(),-9500.0,9500.0));}
        if(change==ItemPositionHasChanged&&!initializing_){try{auto& p=const_cast<CircuitDocument&>(canvas_->document()).part(id);p.x=pos().x();p.y=pos().y();}catch(const std::exception&){} }
        return QGraphicsItem::itemChange(change,v);
    }
    void mousePressEvent(QGraphicsSceneMouseEvent* e)override{before_=canvas_->document();QGraphicsItem::mousePressEvent(e);}
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* e)override{QGraphicsItem::mouseReleaseEvent(e);if(canvas_->editingEnabled()){const auto& a=before_.part(id);const auto& b=canvas_->document().part(id);if(a.x!=b.x||a.y!=b.y){const auto old=before_;QTimer::singleShot(0,canvas_,[c=canvas_,old]{c->recordMove(old);});}}}
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* e)override{if(switchable(kind)){canvas_->toggleButton(id);e->accept();}else QGraphicsItem::mouseDoubleClickEvent(e);}
    void paint(QPainter* p,const QStyleOptionGraphicsItem*,QWidget*)override{
        const auto& part=canvas_->document().part(id);p->setRenderHint(QPainter::Antialiasing);
        if(kind==PartKind::CharacterLcd||kind==PartKind::I2cCharacterLcd){p->setPen(QPen(isSelected()?QColor("#2563eb"):QColor("#215b47"),2));p->setBrush(QColor("#187454"));p->drawRoundedRect(QRectF(0,0,230,130),4,4);p->setBrush(QColor("#172c21"));p->drawRect(QRectF(10,20,210,90));p->setBrush(brightness>.5?QColor("#a8be67"):QColor("#788065"));p->drawRect(QRectF(17,27,196,76));p->setPen(QColor("#d9e4d3"));p->setFont(QFont("Sans Serif",8));p->drawText(QRectF(10,1,210,18),Qt::AlignCenter,part.model=="lcd-20x4"?"HD44780 · 20 × 4":"HD44780 · 16 × 2");int cols=part.model=="lcd-20x4"?20:16,rows=part.model=="lcd-20x4"?4:2;double sx=190.0/(cols*6),sy=68.0/(rows*10);p->setPen(Qt::NoPen);p->setBrush(QColor("#263d22"));for(int y=0;y<rows*10;++y)for(int x=0;x<cols*6;++x)if(visible.pixel(x,y))p->drawRect(QRectF(20+x*sx,31+y*sy,std::max(.5,sx-.3),std::max(.5,sy-.3)));return;}
        if(kind==PartKind::Junction){p->setPen(Qt::NoPen);p->setBrush(isSelected()?QColor("#2563eb"):QColor("#0d9488"));p->drawEllipse(QPointF(0,0),5,5);return;}
        if(kind==PartKind::NetLabel){p->setPen(isSelected()?QColor("#2563eb"):QColor("#0d9488"));p->drawLine(QPointF(0,0),QPointF(140,0));p->setFont(QFont("Sans Serif",9));p->drawText(QRectF(10,-24,135,22),Qt::AlignLeft|Qt::AlignVCenter,QString::fromStdString(part.netName));return;}
        if(realistic_){ComponentDesign::paint(*p,part,state,visible,brightness,isSelected());return;}
        p->setPen(QPen(isSelected()?QColor("#0d9488"):QColor("#cbd5e1"),isSelected()?2.5:1));p->setBrush(QColor("#ffffff"));
        if(kind==PartKind::Esp8266){
            p->scale(blockBoardScale(part),blockBoardScale(part));
            const auto profile=BoardProfile::get(part.model);const auto names=CircuitDocument::pins(part);const int half=(names.size()+1)/2;const int boardHeight=std::max(346,182+(half-1)*24);p->setBrush(QColor(profile.arduino()?"#087b9b":"#143e3c"));p->drawRoundedRect(0,0,210,boardHeight,14,14);p->setPen(Qt::NoPen);p->setBrush(QColor("#dbe5ea"));p->drawRoundedRect(10,30,190,84,6,6);
            p->setPen(QColor("#203b44"));p->setFont(QFont("Segoe UI",13,QFont::Bold));p->drawText(QRectF(12,42,186,30),Qt::AlignCenter,QString::fromStdString(profile.label));p->setFont(QFont("Segoe UI",9));p->drawText(QRectF(38,77,134,24),Qt::AlignCenter,profile.arduino()?"5 V · ATmega328P":part.model.empty()||part.model=="esp8266"?"3.3 V · NodeMCU":"3.3 V · GPIO / ADC");
            p->setBrush(QColor("#0a282d"));p->setPen(Qt::NoPen);p->drawRoundedRect(72,157,65,58,5,5);
            p->setPen(QColor("#d7eeea"));p->setFont(QFont("Segoe UI",8));for(int i=0;i<static_cast<int>(names.size());++i){auto label=QString::fromStdString(names[i]);if(label.startsWith("GPIO"))label.replace("GPIO","IO ");p->drawText(QRectF(i<half?12:135,134+(i%half)*24,64,20),i<half?Qt::AlignLeft:Qt::AlignRight,label);}
            p->setPen(Qt::NoPen);p->setBrush(state.level({id,profile.pinName(profile.builtinLed)})==(profile.arduino()?Level::High:Level::Low)?QColor("#4dd7ff"):QColor("#245f6b"));p->drawEllipse(QPointF(105,242),5,5);p->setBrush(QColor("#a5b6bf"));p->drawRoundedRect(86,boardHeight-21,38,23,3,3);
        }else if(kind==PartKind::Oled){
            p->setBrush(QColor("#173d5b"));p->drawRoundedRect(0,0,292,204,12,12);p->setBrush(QColor("#030d17"));p->setPen(Qt::NoPen);p->drawRoundedRect(18,18,256,128,4,4);
            const auto v=state.voltage.find({id,"VCC"}),g=state.voltage.find({id,"GND"});const bool powered=v!=state.voltage.end()&&g!=state.voltage.end()&&v->second&&g->second&&*v->second-*g->second>=3.0;
            if(powered){auto light=QColor("#8df0eb");light.setAlphaF(brightness);p->setBrush(light);for(int y=0;y<visible.height;++y)for(int x=0;x<128;++x)if(visible.pixel(x,y))p->drawRect(QRectF(18+x*2,18+y*2,1.8,1.8));}
            p->setPen(QColor("#c3e6ef"));p->setFont(QFont("Segoe UI",9));p->drawText(QRectF(12,152,268,24),Qt::AlignCenter,QString("SSD1306 · 128 × %2 · 0x%1").arg(static_cast<int>(part.value),2,16,QChar('0')).arg(CircuitDocument::displayHeight(part)).toUpper());const QStringList pins{"GND","VCC","SCL","SDA"};p->setFont(QFont("Segoe UI",8));for(int i=0;i<4;++i)p->drawText(QRectF(27+i*66,176,42,18),Qt::AlignCenter,pins[i]);
        }else if(kind==PartKind::Breadboard30){p->setBrush(QColor("#e8edf0"));p->drawRoundedRect(0,0,570,260,12,12);p->setPen(QColor("#475569"));p->setFont(QFont("Segoe UI",7));for(int n=1;n<=30;++n)p->drawText(QRectF(12+(n-1)*18,10,18,20),Qt::AlignCenter,QString::number(n));p->drawText(QRectF(10,226,550,24),Qt::AlignCenter,Language::text("a–e / f–j: je fünf Kontakte pro Spalte verbunden · keine Stromschienen"));
        }else{
            p->drawRoundedRect(0,0,150,boundingRect().height()-16,12,12);p->setPen(QColor("#334155"));p->setFont(QFont("Segoe UI",10,QFont::DemiBold));p->drawText(QRectF(8,8,134,22),Qt::AlignCenter,partName(kind));
            if(kind==PartKind::Led||kind==PartKind::InfraredLed){const auto current=state.ledCurrent.contains(id)?state.ledCurrent.at(id):0;p->setPen(Qt::NoPen);p->setBrush(current>1e-5?QColor(QString::fromStdString(part.color)):QColor("#b6bfc9"));if(current>1e-5){auto glow=QColor(QString::fromStdString(part.color));glow.setAlpha(35);p->setBrush(glow);p->drawEllipse(QPointF(75,58),27,27);p->setBrush(QColor(QString::fromStdString(part.color)));}p->drawEllipse(QPointF(75,58),17,17);p->setPen(QColor("#64748b"));p->setFont(QFont("Segoe UI",8));p->drawText(QRectF(8,81,134,18),Qt::AlignCenter,QString("%1 mA · A → K").arg(current*1000,0,'f',1));}
            if(kind==PartKind::Resistor){p->setBrush(QColor("#e7c99b"));p->setPen(Qt::NoPen);p->drawRoundedRect(30,45,90,25,5,5);p->setBrush(QColor("#926a40"));for(int x:{48,61,83,102})p->drawRect(x,45,5,25);p->setPen(QColor("#64748b"));p->setFont(QFont("Segoe UI",8));p->drawText(QRectF(8,81,134,18),Qt::AlignCenter,QString("%1 Ω").arg(part.value));}
            if(switchable(kind)){p->setBrush(part.pressed?QColor("#0d9488"):QColor("#e2e8f0"));p->drawRoundedRect(50,39,50,34,8,8);p->setPen(QColor("#64748b"));p->setFont(QFont("Segoe UI",8));p->drawText(QRectF(8,81,134,18),Qt::AlignCenter,part.pressed?Language::text("Gedrückt"):Language::text("Doppelklick: drücken"));}
            const auto& spec=componentSpec(kind);if(spec.adjustable&&kind!=PartKind::Resistor&&kind!=PartKind::AnalogSource){p->setPen(QColor("#0d9488"));p->setFont(QFont("Segoe UI",12,QFont::DemiBold));p->drawText(QRectF(20,42,110,40),Qt::AlignCenter,QString::number(part.value,'g',4)+QString::fromStdString(spec.unit));}
            if(kind==PartKind::Diode){p->setPen(QPen(QColor("#334155"),3));p->drawLine(35,64,115,64);p->setBrush(QColor("#334155"));p->drawRect(53,52,37,24);p->setPen(QPen(Qt::white,3));p->drawLine(83,52,83,76);}
            if(kind==PartKind::BatteryAA||kind==PartKind::Battery3V||kind==PartKind::Battery9V||kind==PartKind::PowerSupply){p->setPen(QColor("#0d9488"));p->setFont(QFont("Segoe UI",14,QFont::Bold));p->drawText(QRectF(22,40,106,42),Qt::AlignCenter,QString("%1 V").arg(part.value));}
            if(kind==PartKind::ServoMotor||kind==PartKind::DcMotor){p->setPen(QColor("#0d9488"));p->setFont(QFont("Sans Serif",12));auto it=state.partValues.find(id);p->drawText(QRectF(15,38,120,42),Qt::AlignCenter,it==state.partValues.end()?"—":QString::number(it->second,'f',1)+(kind==PartKind::ServoMotor?"°":" rpm"));}
            if(kind==PartKind::InfraredRemote){p->setBrush(QColor("#273548"));p->drawRoundedRect(QRectF(35,35,80,65),8,8);p->setBrush(part.pressed?QColor("#f97316"):QColor("#ef4444"));p->drawEllipse(QPointF(75,58),12,12);p->setPen(Qt::white);p->drawText(QRectF(40,77,70,16),Qt::AlignCenter,"NEC · "+QString::number(int(part.value),16).toUpper());}
            if(kind==PartKind::NfcTestCard){p->setBrush(QColor("#285f9b"));p->drawRoundedRect(QRectF(12,35,130,60),6,6);p->setPen(Qt::white);p->setFont(QFont("Sans Serif",10));p->drawText(QRectF(18,42,118,40),Qt::AlignCenter,part.pressed?"NFC TEST · ON":"NFC TEST · OFF");}
            if(kind==PartKind::TemperatureSensor){p->setPen(QColor("#0d9488"));p->drawText(QRectF(15,38,120,42),Qt::AlignCenter,QString("%1 °C").arg(part.value));}
            if(kind==PartKind::PiezoBuzzer||kind==PartKind::Speaker){p->setBrush(QColor("#343b42"));p->drawEllipse(QPointF(75,61),24,24);p->setBrush(QColor("#111827"));p->drawEllipse(QPointF(75,61),10,10);}
            if(kind==PartKind::Voltmeter){auto a=state.voltage.find({id,"+"}),b=state.voltage.find({id,"-"});p->setPen(QColor("#0d9488"));p->setFont(QFont("Consolas",13,QFont::Bold));p->drawText(QRectF(20,42,110,40),Qt::AlignCenter,a!=state.voltage.end()&&b!=state.voltage.end()&&a->second&&b->second?QString("%1 V").arg(*a->second-*b->second,0,'f',3):"— V");}
            if(kind==PartKind::RGBLed){auto channel=[&](const char* name){auto found=state.ledCurrent.find(id+":"+name);return found==state.ledCurrent.end()?0:static_cast<int>(255*std::sqrt(std::clamp(found->second/0.02,0.0,1.0)));};p->setPen(Qt::NoPen);p->setBrush(QColor(channel("R"),channel("G"),channel("B")));p->drawEllipse(QPointF(75,64),20,20);}
            if(kind==PartKind::SevenSegment){const std::vector<std::pair<QPointF,QPointF>> lines{{{55,38},{95,38}},{{100,42},{100,75}},{{100,83},{100,116}},{{55,120},{95,120}},{{50,83},{50,116}},{{50,42},{50,75}},{{55,79},{95,79}}};for(int i=0;i<7;++i){auto found=state.ledCurrent.find(id+":"+std::string(1,'a'+i));p->setPen(QPen(found!=state.ledCurrent.end()&&found->second>1e-5?QColor("#fb586b"):QColor("#cbd5e1"),5,Qt::SolidLine,Qt::RoundCap));p->drawLine(lines[i].first,lines[i].second);}auto dot=state.ledCurrent.find(id+":dp");p->setBrush(dot!=state.ledCurrent.end()&&dot->second>1e-5?QColor("#fb586b"):QColor("#cbd5e1"));p->drawEllipse(QPointF(115,120),3,3);}
            if(kind==PartKind::AnalogSource){p->setPen(QColor("#0d9488"));p->setFont(QFont("Segoe UI",17,QFont::DemiBold));p->drawText(QRectF(15,36,120,38),Qt::AlignCenter,QString("%1 V").arg(part.value,0,'f',2));p->setPen(QColor("#64748b"));p->setFont(QFont("Segoe UI",8));p->drawText(QRectF(8,81,134,18),Qt::AlignCenter,"OUT                   GND");}
        }
    }
};
CircuitCanvas::CircuitCanvas(CircuitDocument& doc,QWidget* parent):QGraphicsView(parent),document_(doc),history_(this){setScene(new QGraphicsScene(this));setRenderHints(QPainter::Antialiasing|QPainter::SmoothPixmapTransform);setViewportUpdateMode(BoundingRectViewportUpdate);setDragMode(RubberBandDrag);setTransformationAnchor(AnchorUnderMouse);setMinimumSize(180,180);setFrameShape(QFrame::NoFrame);setObjectName("circuitCanvas");connect(scene(),&QGraphicsScene::selectionChanged,this,[this]{emit selectionChanged();QTimer::singleShot(0,this,&CircuitCanvas::refreshBendHandles);});connect(scene(),&QGraphicsScene::changed,this,[this]{redrawWires();});rebuild();}
CircuitCanvas::~CircuitCanvas(){scene()->disconnect(this);}
void CircuitCanvas::resetHistory(){history_.clear();history_.setClean();}
void CircuitCanvas::mutate(const QString& label,std::function<void(CircuitDocument&)> action){if(!editingEnabled_){emit notice(Language::text("Simulation zuerst stoppen, um die Schaltung zu ändern."));return;}try{auto after=document_;action(after);after.validate();history_.push(new Snapshot(this,document_,std::move(after),label));}catch(const std::exception& e){emit notice(QString::fromUtf8(e.what()));}}
void CircuitCanvas::applyDocument(const CircuitDocument& doc){document_=doc;rebuild();emit documentChanged();}
void CircuitCanvas::rebuild(){
    QSignalBlocker sceneSignals(scene());cancelWire();
    cancelWire();
    std::set<std::string> selectedParts,selectedWires;
    for(auto* item:scene()->selectedItems()){
        const auto id=item->data(1).toString().toStdString();
        if(item->data(0)=="part")selectedParts.insert(id);
        if(item->data(0)=="wire")selectedWires.insert(id);
    }
    scene()->clear();parts_.clear();
    for(const auto& part:document_.parts()){
        auto* item=new PartItem(this,part);
        item->setFlag(QGraphicsItem::ItemIsMovable,editingEnabled_&&!wireMode_);
        scene()->addItem(item);parts_[part.id]=item;
        item->setSelected(selectedParts.contains(part.id));
    }
    for(const auto& wire:document_.wires()){
        auto* item=new WireItem(wire,routedWire(wire));scene()->addItem(item);
        item->setSelected(selectedWires.contains(wire.id));
    }
    scene()->setSceneRect(scene()->itemsBoundingRect().adjusted(-500,-350,600,400));
    viewport()->update();sceneSignals.unblock();emit selectionChanged();
}
QPainterPath CircuitCanvas::routedWire(const CircuitWire& wire) const {
    QPainterPath path(pinPosition(wire.a));
    for(const auto& point:wire.points)path.lineTo(point.x,point.y);
    path.lineTo(pinPosition(wire.b));return path;
}

void CircuitCanvas::redrawWires(){for(auto* item:scene()->items())if(item->data(0)=="wire"){const auto id=item->data(1).toString().toStdString();for(const auto& w:document_.wires())if(w.id==id){auto* path=static_cast<QGraphicsPathItem*>(item);auto rendered=w;for(auto* handle:scene()->items())if(handle->data(0)=="bend"&&handle->data(1).toString().toStdString()==w.id){const auto i=handle->data(2).toInt();if(i>=0&&i<static_cast<int>(rendered.points.size()))rendered.points[i]={handle->pos().x(),handle->pos().y()};}const auto next=routedWire(rendered);if(next!=path->path())path->setPath(next);break;}}}
void CircuitCanvas::setEditingEnabled(bool enabled){editingEnabled_=enabled;cancelWire();for(auto& [id,item]:parts_){(void)id;item->setFlag(QGraphicsItem::ItemIsMovable,enabled&&!wireMode_);}history_.setActive(enabled);refreshBendHandles();}
void CircuitCanvas::setWireMode(bool enabled){wireMode_=enabled;cancelWire();setDragMode(enabled?NoDrag:RubberBandDrag);for(auto& [id,item]:parts_){(void)id;item->setFlag(QGraphicsItem::ItemIsMovable,editingEnabled_&&!enabled);}}
QPointF CircuitCanvas::pinPosition(const Endpoint& ep)const{return parts_.at(ep.part)->mapToScene(localPin(document_.part(ep.part),ep.pin));}
void CircuitCanvas::connectPin(const Endpoint& pin){if(!editingEnabled_){emit notice(Language::text("Simulation zuerst stoppen, um Kabel zu ändern."));return;}if(!wireStart_){wireStart_=pin;preview_=scene()->addPath(wirePath(pinPosition(pin),pinPosition(pin)),QPen(wireColor_,2,Qt::DashLine));preview_->setZValue(4);emit notice(Language::text("Zielpin anklicken. Esc bricht das Kabel ab."));return;}const auto a=*wireStart_;const auto bends=pendingBends_;cancelWire();if(a==pin)return;const auto color=wireColor_.name().toStdString();QTimer::singleShot(0,this,[this,a,pin,color,bends]{mutate(Language::text("Kabel verbinden"),[&](auto& doc){doc.connect(a,pin,color);const_cast<CircuitWire&>(doc.wires().back()).points=bends;});});}
void CircuitCanvas::cancelWire(){wireStart_.reset();pendingBends_.clear();if(preview_){scene()->removeItem(preview_);delete preview_;preview_=nullptr;}}
void CircuitCanvas::addPart(PartKind kind){const auto pos=mapToScene(viewport()->rect().center());mutate(Language::text("Bauteil hinzufügen"),[&](auto& doc){doc.addPart(kind,pos.x()-70,pos.y()-50);});}
void CircuitCanvas::removeSelected(){std::vector<std::pair<QString,QString>> selected;for(auto* item:scene()->selectedItems())selected.emplace_back(item->data(0).toString(),item->data(1).toString());mutate(Language::text("Auswahl löschen"),[&](auto& doc){for(const auto& [kind,id]:selected)if(kind=="wire")doc.removeWire(id.toStdString());for(const auto& [kind,id]:selected)if(kind=="part")doc.removePart(id.toStdString());});}
void CircuitCanvas::rotateSelected(){
    std::vector<std::string> selected;
    for(auto* item:scene()->selectedItems())if(item->data(0)=="part")selected.push_back(item->data(1).toString().toStdString());
    if(selected.empty())return;
    mutate(Language::text("Bauteile rotieren"),[&](auto& doc){
        for(const auto& id:selected){auto& part=doc.part(id);part.rotation=(part.rotation+90)%360;}
    });
}
void CircuitCanvas::recordMove(const CircuitDocument& before){history_.push(new Snapshot(this,before,document_,Language::text("Bauteil verschieben")));}
void CircuitCanvas::toggleButton(const std::string& id){if(editingEnabled_){mutate(Language::text("Kontakt umschalten"),[&](auto& doc){doc.part(id).pressed=!doc.part(id).pressed;});return;}auto& p=document_.part(id);p.pressed=!p.pressed;parts_.at(id)->update();emit inputValueChanged();emit runtimeInputChanged();}
void CircuitCanvas::fitCircuit(){fitInView(scene()->itemsBoundingRect().adjusted(-50,-50,50,50),Qt::KeepAspectRatio);if(transform().m11()>1.4){resetTransform();scale(1.4,1.4);}}
void CircuitCanvas::zoom(double factor){const auto target=transform().m11()*factor;if(target>=0.25&&target<=3.5)scale(factor,factor);}
void CircuitCanvas::updateSimulation(const CircuitState& state,const std::map<std::string,OledDisplay>& displays){for(auto& [id,item]:parts_){item->state=state;if(displays.contains(id)){item->visible=displays.at(id).frame();item->brightness=displays.at(id).brightness();}}viewport()->update();}
QString CircuitCanvas::selectedPart()const{for(auto* item:scene()->selectedItems())if(item->data(0)=="part")return item->data(1).toString();return {};}
void CircuitCanvas::selectPart(const std::string& id){scene()->clearSelection();if(parts_.contains(id))parts_.at(id)->setSelected(true);}
void CircuitCanvas::mouseMoveEvent(QMouseEvent* e){if(panning_){const auto delta=e->pos()-panOrigin_;panOrigin_=e->pos();horizontalScrollBar()->setValue(horizontalScrollBar()->value()-delta.x());verticalScrollBar()->setValue(verticalScrollBar()->value()-delta.y());e->accept();return;}QGraphicsView::mouseMoveEvent(e);if(preview_&&wireStart_)preview_->setPath(previewPath(mapToScene(e->pos())));}
void CircuitCanvas::mousePressEvent(QMouseEvent* e){if(e->button()==Qt::RightButton){auto* item=itemAt(e->pos());QMenu menu(this);if(auto* pin=dynamic_cast<PinItem*>(item)){auto ep=pin->endpoint();menu.addAction(Language::text("Netz hervorheben"),this,[this,ep]{highlightNet(ep);});if(editingEnabled_)menu.addAction(Language::text("Netzlabel setzen…"),this,[this,ep]{bool ok=false;auto name=QInputDialog::getText(this,Language::text("Netzlabel"),Language::text("Netzname (gleicher Name verbindet Pins)"),QLineEdit::Normal,{},&ok);if(ok&&!name.isEmpty()){auto point=pinPosition(ep)+QPointF(30,-30);mutate(Language::text("Netzlabel setzen"),[&](auto& doc){auto id=doc.addPart(PartKind::NetLabel,point.x(),point.y());doc.part(id).netName=name.toStdString();doc.connect(ep,{id,"NET"});});}});}else if(item&&item->data(0)=="wire"&&editingEnabled_){auto id=item->data(1).toString().toStdString();auto pos=mapToScene(e->pos());menu.addAction(Language::text("Abzweigung erstellen"),this,[this,id,pos]{branchWire(id,pos);});}if(!menu.isEmpty()){menu.exec(e->globalPosition().toPoint());e->accept();return;}}
if(e->button()==Qt::LeftButton){for(auto* candidate:items(e->pos()))if(auto* pin=dynamic_cast<PinItem*>(candidate)){connectPin(pin->endpoint());e->accept();return;}}if(e->button()==Qt::LeftButton&&wireStart_&&!dynamic_cast<PinItem*>(itemAt(e->pos()))){if(pendingBends_.size()<64){auto point=mapToScene(e->pos());point=snapped(point);pendingBends_.push_back({point.x(),point.y()});if(preview_)preview_->setPath(previewPath(point));}e->accept();return;}if(e->button()==Qt::MiddleButton){panning_=true;panOrigin_=e->pos();viewport()->setCursor(Qt::ClosedHandCursor);e->accept();return;}QGraphicsView::mousePressEvent(e);}
void CircuitCanvas::mouseReleaseEvent(QMouseEvent* e){if(e->button()==Qt::MiddleButton&&panning_){panning_=false;viewport()->unsetCursor();e->accept();return;}QGraphicsView::mouseReleaseEvent(e);}
void CircuitCanvas::keyPressEvent(QKeyEvent* e){if(e->key()==Qt::Key_Escape){cancelWire();e->accept();}else if(e->key()==Qt::Key_Delete){removeSelected();e->accept();}else QGraphicsView::keyPressEvent(e);}
void CircuitCanvas::wheelEvent(QWheelEvent* e){if(e->modifiers()&Qt::ControlModifier){zoom(e->angleDelta().y()>0?1.15:1/1.15);e->accept();}else QGraphicsView::wheelEvent(e);}
QPointF CircuitCanvas::snapped(QPointF pos)const{if(!snap_||(QApplication::keyboardModifiers()&Qt::AltModifier))return pos;return {std::round(pos.x()/gridSize_)*gridSize_,std::round(pos.y()/gridSize_)*gridSize_};}
void CircuitCanvas::drawBackground(QPainter* p,const QRectF& r){p->fillRect(r,palette().color(QPalette::Window));p->setPen(QPen(palette().color(QPalette::Mid),1));double visibleGrid=gridSize_;while(visibleGrid*transform().m11()<6)visibleGrid*=2;for(double x=std::floor(r.left()/visibleGrid)*visibleGrid;x<r.right();x+=visibleGrid)for(double y=std::floor(r.top()/visibleGrid)*visibleGrid;y<r.bottom();y+=visibleGrid)p->drawPoint(QPointF(x,y));}

QPainterPath CircuitCanvas::previewPath(QPointF end)const{QPainterPath path(pinPosition(*wireStart_));for(const auto& point:pendingBends_)path.lineTo(point.x,point.y);path.lineTo(end);return path;}
void CircuitCanvas::refreshBendHandles(){for(auto* item:scene()->items())if(item->data(0)=="bend")delete item;if(!editingEnabled_)return;for(auto* item:scene()->selectedItems())if(item->data(0)=="wire"){const auto id=item->data(1).toString().toStdString();for(const auto& wire:document_.wires())if(wire.id==id)for(std::size_t i=0;i<wire.points.size();++i)scene()->addItem(new BendHandle(this,id,i,{wire.points[i].x,wire.points[i].y}));}}
void CircuitCanvas::moveBend(const std::string& id,std::size_t index,QPointF point){mutate(Language::text("Kabelknick verschieben"),[&](auto& doc){for(auto& wire:const_cast<std::vector<CircuitWire>&>(doc.wires()))if(wire.id==id){auto target=snapped(point);wire.points.at(index)={target.x(),target.y()};}});}
void CircuitCanvas::addBend(const std::string& id,QPointF point){mutate(Language::text("Kabelknick hinzufügen"),[&](auto& doc){for(auto& wire:const_cast<std::vector<CircuitWire>&>(doc.wires()))if(wire.id==id){if(wire.points.size()>=64)throw std::runtime_error(Language::text("Maximal 64 Kabelknicke").toStdString());auto position=wire.points.end();double distance=1e100;QPointF previous=pinPosition(wire.a);for(std::size_t i=0;i<=wire.points.size();++i){const QPointF next=i<wire.points.size()?QPointF(wire.points[i].x,wire.points[i].y):pinPosition(wire.b);const double candidate=QLineF(previous,point).length()+QLineF(point,next).length()-QLineF(previous,next).length();if(candidate<distance){distance=candidate;position=wire.points.begin()+i;}previous=next;}auto target=snapped(point);wire.points.insert(position,{target.x(),target.y()});}});}
void CircuitCanvas::mouseDoubleClickEvent(QMouseEvent* event){auto* item=itemAt(event->pos());if(item&&item->data(0)=="wire"){addBend(item->data(1).toString().toStdString(),mapToScene(event->pos()));event->accept();return;}QGraphicsView::mouseDoubleClickEvent(event);}

void CircuitCanvas::changeValue(const std::string& id,double value){if(editingEnabled_){mutate(Language::text("Bauteilwert ändern"),[&](auto& doc){doc.part(id).value=value;});return;}auto next=document_;next.part(id).value=value;try{next.validate();document_=std::move(next);emit inputValueChanged();emit runtimeInputChanged();viewport()->update();}catch(const std::exception& error){emit notice(QString::fromUtf8(error.what()));}}

void CircuitCanvas::copySelected(){
    SavedProject fragment;fragment.circuit.removePart("board");fragment.sketch.order={"sketch.ino"};fragment.sketch.files["sketch.ino"]="void setup(){} void loop(){}";std::set<std::string> ids;
    for(auto* item:scene()->selectedItems())if(item->data(0)=="part"){auto id=item->data(1).toString().toStdString();ids.insert(id);fragment.circuit.insertPart(document_.part(id));}
    if(ids.empty())return;for(const auto& w:document_.wires())if(ids.contains(w.a.part)&&ids.contains(w.b.part)){fragment.circuit.connect(w.a,w.b,w.color);const_cast<CircuitWire&>(fragment.circuit.wires().back()).points=w.points;}
    if(clipboardSketches){auto sketches=clipboardSketches();for(auto it=sketches.begin();it!=sketches.end();++it)if(ids.contains(it.key().toStdString()))fragment.boardSketches[it.key()]=it.value();}
    auto* data=new QMimeData;data->setData("application/x-thinkcad-circuit",ProjectIO::serialize(fragment));QApplication::clipboard()->setMimeData(data);
}
void CircuitCanvas::paste(){
    if(!editingEnabled_)return;const auto* mime=QApplication::clipboard()->mimeData();if(!mime->hasFormat("application/x-thinkcad-circuit"))return;
    try{auto fragment=ProjectIO::deserialize(mime->data("application/x-thinkcad-circuit"));if(fragment.circuit.parts().empty())return;std::vector<std::string> created;QMap<QString,SketchProject> imported;mutate(Language::text("Bauteile einfügen"),[&](auto& doc){std::map<std::string,std::string> names;double left=1e9,top=1e9;for(const auto& p:fragment.circuit.parts()){left=std::min(left,p.x);top=std::min(top,p.y);}const auto center=snapped(mapToScene(viewport()->rect().center()));double dx=center.x()-left+30,dy=center.y()-top+30;
        for(const auto& p:fragment.circuit.parts()){auto id=doc.addPart(p.kind,p.x+dx,p.y+dy);auto copy=p;copy.id=id;copy.x+=dx;copy.y+=dy;doc.part(id)=copy;names[p.id]=id;created.push_back(id);if(fragment.boardSketches.contains(QString::fromStdString(p.id)))imported[QString::fromStdString(id)]=fragment.boardSketches.value(QString::fromStdString(p.id));}for(const auto& w:fragment.circuit.wires()){doc.connect({names.at(w.a.part),w.a.pin},{names.at(w.b.part),w.b.pin},w.color);auto& wire=const_cast<CircuitWire&>(doc.wires().back());for(const auto& point:w.points)wire.points.push_back({point.x+dx,point.y+dy});}});if(importClipboardSketches&&!imported.isEmpty()&&std::all_of(created.begin(),created.end(),[&](const auto& id){return parts_.contains(id);}))importClipboardSketches(imported);scene()->clearSelection();for(const auto& id:created)if(parts_.contains(id))parts_.at(id)->setSelected(true);
    }catch(const std::exception& e){emit notice(QString::fromUtf8(e.what()));}
}
void CircuitCanvas::duplicateSelected(){copySelected();paste();}
void CircuitCanvas::alignSelected(bool horizontal){std::vector<std::string> ids;for(auto* item:scene()->selectedItems())if(item->data(0)=="part")ids.push_back(item->data(1).toString().toStdString());if(ids.size()<2)return;mutate(Language::text("Bauteile ausrichten"),[&](auto& doc){double sum=0;for(const auto& id:ids)sum+=horizontal?doc.part(id).y:doc.part(id).x;const double target=sum/ids.size();for(const auto& id:ids){if(horizontal)doc.part(id).y=target;else doc.part(id).x=target;}});}
void CircuitCanvas::distributeSelected(bool horizontal){std::vector<std::string> ids;for(auto* item:scene()->selectedItems())if(item->data(0)=="part")ids.push_back(item->data(1).toString().toStdString());if(ids.size()<3)return;auto coordinate=[&](const std::string& id){return horizontal?document_.part(id).x:document_.part(id).y;};std::sort(ids.begin(),ids.end(),[&](const auto& a,const auto& b){return coordinate(a)<coordinate(b);});const double start=coordinate(ids.front()),step=(coordinate(ids.back())-start)/(ids.size()-1);mutate(Language::text("Bauteile verteilen"),[&](auto& doc){for(std::size_t i=0;i<ids.size();++i){if(horizontal)doc.part(ids[i]).x=start+i*step;else doc.part(ids[i]).y=start+i*step;}});}
void CircuitCanvas::highlightNet(const Endpoint& ep){scene()->clearSelection();for(auto* item:scene()->items())if(item->data(0)=="wire"){auto id=item->data(1).toString().toStdString();for(const auto& wire:document_.wires())if(wire.id==id&&document_.connected(ep,wire.a))item->setSelected(true);}}
void CircuitCanvas::branchWire(const std::string& id,QPointF pos){
    std::string junction;mutate(Language::text("Abzweigung erstellen"),[&](auto& doc){auto it=std::find_if(doc.wires().begin(),doc.wires().end(),[&](const auto& w){return w.id==id;});if(it==doc.wires().end())throw std::runtime_error("Unknown wire");auto old=*it;std::vector<QPointF> path{pinPosition(old.a)};for(const auto& b:old.points)path.push_back({b.x,b.y});path.push_back(pinPosition(old.b));double best=1e100;std::size_t index=0;QPointF projection;
        for(std::size_t i=0;i+1<path.size();++i){auto delta=path[i+1]-path[i];double length=QPointF::dotProduct(delta,delta);double t=length?std::clamp(QPointF::dotProduct(pos-path[i],delta)/length,0.0,1.0):0;auto point=path[i]+delta*t;double distance=QLineF(point,pos).length();if(distance<best){best=distance;index=i;projection=point;}}
        junction=doc.addPart(PartKind::Junction,projection.x(),projection.y());doc.removeWire(id);doc.connect(old.a,{junction,"NET"},old.color);auto& first=const_cast<CircuitWire&>(doc.wires().back());first.points.assign(old.points.begin(),old.points.begin()+index);doc.connect({junction,"NET"},old.b,old.color);auto& second=const_cast<CircuitWire&>(doc.wires().back());second.points.assign(old.points.begin()+index,old.points.end());});if(!junction.empty()){selectPart(junction);connectPin({junction,"NET"});}
}
