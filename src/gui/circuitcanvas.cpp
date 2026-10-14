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
QPainterPath wirePath(QPointF a,QPointF b) {QPainterPath p(a);const auto mid=(a.x()+b.x())/2;p.lineTo(mid,a.y());p.lineTo(mid,b.y());p.lineTo(b);return p;}
QString partName(PartKind k){switch(k){case PartKind::Esp8266:return "ESP8266";case PartKind::Oled:return "OLED 128 × 64";case PartKind::Led:return "LED";case PartKind::Resistor:return "Widerstand";case PartKind::Button:return "Taster";case PartKind::AnalogSource:return "Analogquelle";}return {};}
QPointF localPin(PartKind kind,const std::string& pin) {
    const auto names=CircuitDocument::pins(kind);const int index=static_cast<int>(std::find(names.begin(),names.end(),pin)-names.begin());
    if(kind==PartKind::Esp8266)return index<8?QPointF(0,142+index*24):QPointF(210,142+(index-8)*24);
    if(kind==PartKind::Oled)return {47.0+index*66,200};
    return {index?150.0:0.0,64};
}
class WireItem final:public QGraphicsPathItem {
public:
    WireItem(const CircuitWire& wire,const QPainterPath& path):QGraphicsPathItem(path){setData(0,"wire");setData(1,QString::fromStdString(wire.id));setFlags(ItemIsSelectable);setZValue(-1);setPen(QPen(QColor(QString::fromStdString(wire.color)),2.8,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));setToolTip(QString::fromStdString(wire.a.part+"."+wire.a.pin+" → "+wire.b.part+"."+wire.b.pin));}
    QPainterPath shape() const override{QPainterPathStroker s;s.setWidth(14);return s.createStroke(path());}
    void paint(QPainter* p,const QStyleOptionGraphicsItem*,QWidget*)override{auto color=pen();if(isSelected()){color.setColor(QColor("#2563eb"));color.setWidthF(5);}p->setPen(color);p->setBrush(Qt::NoBrush);p->drawPath(path());}
};
class Snapshot final:public QUndoCommand {
    CircuitCanvas* canvas_;CircuitDocument before_,after_;
public:
    Snapshot(CircuitCanvas* c,CircuitDocument before,CircuitDocument after,const QString& label):QUndoCommand(label),canvas_(c),before_(std::move(before)),after_(std::move(after)){}
    void undo()override{canvas_->applyDocument(before_);}void redo()override{canvas_->applyDocument(after_);}
};
class PinItem final:public QGraphicsEllipseItem {
    CircuitCanvas* canvas_;Endpoint endpoint_;
public:
    PinItem(CircuitCanvas* c,Endpoint e,QGraphicsItem* parent):QGraphicsEllipseItem(-7,-7,14,14,parent),canvas_(c),endpoint_(std::move(e)){setBrush(QColor("#ffffff"));setPen(QPen(QColor("#0d9488"),2));setZValue(3);setCursor(Qt::CrossCursor);setToolTip(QString::fromStdString(endpoint_.pin)+" — zum Verbinden klicken");}
    void mousePressEvent(QGraphicsSceneMouseEvent* e)override{if(e->button()==Qt::LeftButton){canvas_->connectPin(endpoint_);e->accept();}else QGraphicsEllipseItem::mousePressEvent(e);}
};
}
class PartItem final:public QGraphicsItem {
    CircuitCanvas* canvas_;
    CircuitDocument before_;
    bool initializing_=true;
public:
    std::string id;PartKind kind;CircuitState state;Frame visible;
    PartItem(CircuitCanvas* c,const CircuitPart& p):canvas_(c),id(p.id),kind(p.kind){setData(0,"part");setData(1,QString::fromStdString(id));setFlags(ItemIsSelectable|ItemIsMovable|ItemSendsGeometryChanges);setZValue(1);setPos(p.x,p.y);initializing_=false;setCursor(Qt::OpenHandCursor);for(const auto& pin:CircuitDocument::pins(kind)){auto* port=new PinItem(c,{id,pin},this);port->setPos(localPin(kind,pin));}}
    QRectF boundingRect()const override{return kind==PartKind::Esp8266?QRectF(-10,-10,230,370):kind==PartKind::Oled?QRectF(-10,-10,310,224):QRectF(-10,-10,170,120);}
    QVariant itemChange(GraphicsItemChange change,const QVariant& v)override{
        if(change==ItemPositionChange&&!initializing_){auto pos=v.toPointF();return QPointF(std::clamp(std::round(pos.x()/10)*10,-9500.0,9500.0),std::clamp(std::round(pos.y()/10)*10,-9500.0,9500.0));}
        if(change==ItemPositionHasChanged&&!initializing_){try{auto& p=const_cast<CircuitDocument&>(canvas_->document()).part(id);p.x=pos().x();p.y=pos().y();}catch(const std::exception&){} }
        return QGraphicsItem::itemChange(change,v);
    }
    void mousePressEvent(QGraphicsSceneMouseEvent* e)override{before_=canvas_->document();QGraphicsItem::mousePressEvent(e);}
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* e)override{QGraphicsItem::mouseReleaseEvent(e);if(canvas_->editingEnabled()){const auto& a=before_.part(id);const auto& b=canvas_->document().part(id);if(a.x!=b.x||a.y!=b.y){const auto old=before_;QTimer::singleShot(0,canvas_,[c=canvas_,old]{c->recordMove(old);});}}}
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* e)override{if(kind==PartKind::Button){canvas_->toggleButton(id);e->accept();}else QGraphicsItem::mouseDoubleClickEvent(e);}
    void paint(QPainter* p,const QStyleOptionGraphicsItem*,QWidget*)override{
        const auto& part=canvas_->document().part(id);p->setRenderHint(QPainter::Antialiasing);
        p->setPen(QPen(isSelected()?QColor("#0d9488"):QColor("#cbd5e1"),isSelected()?2.5:1));p->setBrush(QColor("#ffffff"));
        if(kind==PartKind::Esp8266){
            p->setBrush(QColor("#143e3c"));p->drawRoundedRect(0,0,210,346,14,14);p->setPen(Qt::NoPen);p->setBrush(QColor("#dbe5ea"));p->drawRoundedRect(38,30,134,84,6,6);
            p->setPen(QColor("#203b44"));p->setFont(QFont("Segoe UI",13,QFont::Bold));p->drawText(QRectF(38,42,134,30),Qt::AlignCenter,"ESP8266");p->setFont(QFont("Segoe UI",9));p->drawText(QRectF(38,77,134,24),Qt::AlignCenter,"3.3 V · NodeMCU");
            p->setBrush(QColor("#0a282d"));p->setPen(Qt::NoPen);p->drawRoundedRect(72,157,65,58,5,5);
            const QStringList labels{"3V3","GND","D0 · 16","D1 · 5","D2 · 4","D3 · 0","D4 · 2","A0","GND","D5 · 14","D6 · 12","D7 · 13","D8 · 15","RX · 3","TX · 1","3V3"};
            p->setPen(QColor("#d7eeea"));p->setFont(QFont("Segoe UI",8));for(int i=0;i<16;++i)p->drawText(QRectF(i<8?12:135,134+(i%8)*24,64,20),i<8?Qt::AlignLeft:Qt::AlignRight,labels[i]);
            p->setPen(Qt::NoPen);p->setBrush(state.level({id,"GPIO2"})==Level::Low?QColor("#4dd7ff"):QColor("#245f6b"));p->drawEllipse(QPointF(105,242),5,5);p->setBrush(QColor("#a5b6bf"));p->drawRoundedRect(86,325,38,23,3,3);
        }else if(kind==PartKind::Oled){
            p->setBrush(QColor("#173d5b"));p->drawRoundedRect(0,0,292,204,12,12);p->setBrush(QColor("#030d17"));p->setPen(Qt::NoPen);p->drawRoundedRect(18,18,256,128,4,4);
            const auto v=state.voltage.find({id,"VCC"}),g=state.voltage.find({id,"GND"});const bool powered=v!=state.voltage.end()&&g!=state.voltage.end()&&v->second&&g->second&&*v->second-*g->second>=3.0;
            if(powered){p->setBrush(QColor("#8df0eb"));for(int y=0;y<64;++y)for(int x=0;x<128;++x)if(visible.pixel(x,y))p->drawRect(QRectF(18+x*2,18+y*2,1.8,1.8));}
            p->setPen(QColor("#c3e6ef"));p->setFont(QFont("Segoe UI",9));p->drawText(QRectF(12,152,268,24),Qt::AlignCenter,QString("SSD1306 · 128 × 64 · 0x%1").arg(static_cast<int>(part.value),2,16,QChar('0')).toUpper());const QStringList pins{"GND","VCC","SCL","SDA"};p->setFont(QFont("Segoe UI",8));for(int i=0;i<4;++i)p->drawText(QRectF(27+i*66,176,42,18),Qt::AlignCenter,pins[i]);
        }else{
            p->drawRoundedRect(0,0,150,104,12,12);p->setPen(QColor("#334155"));p->setFont(QFont("Segoe UI",10,QFont::DemiBold));p->drawText(QRectF(8,8,134,22),Qt::AlignCenter,partName(kind));
            if(kind==PartKind::Led){const auto current=state.ledCurrent.contains(id)?state.ledCurrent.at(id):0;p->setPen(Qt::NoPen);p->setBrush(current>1e-5?QColor("#fb586b"):QColor("#b6bfc9"));if(current>1e-5){p->setBrush(QColor(251,88,107,35));p->drawEllipse(QPointF(75,58),27,27);p->setBrush(QColor("#fb586b"));}p->drawEllipse(QPointF(75,58),17,17);p->setPen(QColor("#64748b"));p->setFont(QFont("Segoe UI",8));p->drawText(QRectF(8,81,134,18),Qt::AlignCenter,QString("%1 mA · A → K").arg(current*1000,0,'f',1));}
            if(kind==PartKind::Resistor){p->setBrush(QColor("#e7c99b"));p->setPen(Qt::NoPen);p->drawRoundedRect(30,45,90,25,5,5);p->setBrush(QColor("#926a40"));for(int x:{48,61,83,102})p->drawRect(x,45,5,25);p->setPen(QColor("#64748b"));p->setFont(QFont("Segoe UI",8));p->drawText(QRectF(8,81,134,18),Qt::AlignCenter,QString("%1 Ω").arg(part.value));}
            if(kind==PartKind::Button){p->setBrush(part.pressed?QColor("#0d9488"):QColor("#e2e8f0"));p->drawRoundedRect(50,39,50,34,8,8);p->setPen(QColor("#64748b"));p->setFont(QFont("Segoe UI",8));p->drawText(QRectF(8,81,134,18),Qt::AlignCenter,part.pressed?"Gedrückt":"Doppelklick: drücken");}
            if(kind==PartKind::AnalogSource){p->setPen(QColor("#0d9488"));p->setFont(QFont("Segoe UI",17,QFont::DemiBold));p->drawText(QRectF(15,36,120,38),Qt::AlignCenter,QString("%1 V").arg(part.value,0,'f',2));p->setPen(QColor("#64748b"));p->setFont(QFont("Segoe UI",8));p->drawText(QRectF(8,81,134,18),Qt::AlignCenter,"OUT                   GND");}
        }
    }
};
CircuitCanvas::CircuitCanvas(CircuitDocument& doc,QWidget* parent):QGraphicsView(parent),document_(doc),history_(this){setScene(new QGraphicsScene(this));setRenderHints(QPainter::Antialiasing|QPainter::SmoothPixmapTransform);setViewportUpdateMode(BoundingRectViewportUpdate);setDragMode(RubberBandDrag);setTransformationAnchor(AnchorUnderMouse);setMinimumSize(180,180);setFrameShape(QFrame::NoFrame);setObjectName("circuitCanvas");connect(scene(),&QGraphicsScene::selectionChanged,this,&CircuitCanvas::selectionChanged);connect(scene(),&QGraphicsScene::changed,this,[this]{redrawWires();});rebuild();}
CircuitCanvas::~CircuitCanvas(){scene()->disconnect(this);}
void CircuitCanvas::resetHistory(){history_.clear();history_.setClean();}
void CircuitCanvas::mutate(const QString& label,std::function<void(CircuitDocument&)> action){if(!editingEnabled_){emit notice("Simulation zuerst stoppen, um die Schaltung zu ändern.");return;}try{auto after=document_;action(after);after.validate();history_.push(new Snapshot(this,document_,std::move(after),label));}catch(const std::exception& e){emit notice(QString::fromUtf8(e.what()));}}
void CircuitCanvas::applyDocument(const CircuitDocument& doc){document_=doc;rebuild();emit documentChanged();}
void CircuitCanvas::rebuild(){cancelWire();const auto selected=selectedPart();scene()->clear();parts_.clear();for(const auto& p:document_.parts()){auto* item=new PartItem(this,p);item->setFlag(QGraphicsItem::ItemIsMovable,editingEnabled_&&!wireMode_);scene()->addItem(item);parts_[p.id]=item;}for(const auto& w:document_.wires())scene()->addItem(new WireItem(w,routedWire(w)));scene()->setSceneRect(scene()->itemsBoundingRect().adjusted(-500,-350,600,400));if(!selected.isEmpty())selectPart(selected.toStdString());viewport()->update();}
QPainterPath CircuitCanvas::routedWire(const CircuitWire& wire) const {
    const auto a=pinPosition(wire.a),b=pinPosition(wire.b);
    const auto index=static_cast<int>(&wire-document_.wires().data());
    const auto offset=24.0+12.0*(index%10);
    auto exit=[&](const Endpoint& ep,QPointF point){
        const auto kind=document_.part(ep.part).kind;
        if(kind==PartKind::Oled)return point+QPointF(0,offset);
        return point+QPointF(localPin(kind,ep.pin).x()==0?-offset:offset,0);
    };
    const auto first=exit(wire.a,a),last=exit(wire.b,b);
    const auto bottom=std::max(parts_.at(wire.a.part)->sceneBoundingRect().bottom(),parts_.at(wire.b.part)->sceneBoundingRect().bottom())+offset;
    QPainterPath path(a);path.lineTo(first);path.lineTo(first.x(),bottom);path.lineTo(last.x(),bottom);path.lineTo(last);path.lineTo(b);return path;
}
void CircuitCanvas::redrawWires(){for(auto* item:scene()->items())if(item->data(0)=="wire"){const auto id=item->data(1).toString().toStdString();for(const auto& w:document_.wires())if(w.id==id){auto* path=static_cast<QGraphicsPathItem*>(item);const auto next=routedWire(w);if(next!=path->path())path->setPath(next);break;}}}
void CircuitCanvas::setEditingEnabled(bool enabled){editingEnabled_=enabled;cancelWire();for(auto& [id,item]:parts_){(void)id;item->setFlag(QGraphicsItem::ItemIsMovable,enabled&&!wireMode_);}history_.setActive(enabled);}
void CircuitCanvas::setWireMode(bool enabled){wireMode_=enabled;cancelWire();setDragMode(enabled?NoDrag:RubberBandDrag);for(auto& [id,item]:parts_){(void)id;item->setFlag(QGraphicsItem::ItemIsMovable,editingEnabled_&&!enabled);}}
QPointF CircuitCanvas::pinPosition(const Endpoint& ep)const{return parts_.at(ep.part)->mapToScene(localPin(document_.part(ep.part).kind,ep.pin));}
void CircuitCanvas::connectPin(const Endpoint& pin){if(!editingEnabled_){emit notice("Simulation zuerst stoppen, um Kabel zu ändern.");return;}if(!wireStart_){wireStart_=pin;preview_=scene()->addPath(wirePath(pinPosition(pin),pinPosition(pin)),QPen(wireColor_,2,Qt::DashLine));preview_->setZValue(4);emit notice("Zielpin anklicken. Esc bricht das Kabel ab.");return;}const auto a=*wireStart_;cancelWire();if(a==pin)return;const auto color=wireColor_.name().toStdString();QTimer::singleShot(0,this,[this,a,pin,color]{mutate("Kabel verbinden",[&](auto& doc){doc.connect(a,pin,color);});});}
void CircuitCanvas::cancelWire(){wireStart_.reset();if(preview_){scene()->removeItem(preview_);delete preview_;preview_=nullptr;}}
void CircuitCanvas::addPart(PartKind kind){const auto pos=mapToScene(viewport()->rect().center());mutate("Bauteil hinzufügen",[&](auto& doc){doc.addPart(kind,pos.x()-70,pos.y()-50);});}
void CircuitCanvas::removeSelected(){std::vector<std::pair<QString,QString>> selected;for(auto* item:scene()->selectedItems())selected.emplace_back(item->data(0).toString(),item->data(1).toString());mutate("Auswahl löschen",[&](auto& doc){for(const auto& [kind,id]:selected)if(kind=="wire")doc.removeWire(id.toStdString());for(const auto& [kind,id]:selected)if(kind=="part"&&id!="board")doc.removePart(id.toStdString());});}
void CircuitCanvas::recordMove(const CircuitDocument& before){history_.push(new Snapshot(this,before,document_,"Bauteil verschieben"));}
void CircuitCanvas::toggleButton(const std::string& id){auto& p=document_.part(id);p.pressed=!p.pressed;parts_.at(id)->update();emit runtimeInputChanged();}
void CircuitCanvas::fitCircuit(){fitInView(scene()->itemsBoundingRect().adjusted(-50,-50,50,50),Qt::KeepAspectRatio);if(transform().m11()>1.4){resetTransform();scale(1.4,1.4);}}
void CircuitCanvas::zoom(double factor){const auto target=transform().m11()*factor;if(target>=0.25&&target<=3.5)scale(factor,factor);}
void CircuitCanvas::updateSimulation(const CircuitState& state,const std::map<std::string,OledDisplay>& displays){for(auto& [id,item]:parts_){item->state=state;if(displays.contains(id))item->visible=displays.at(id).frame();}viewport()->update();}
QString CircuitCanvas::selectedPart()const{for(auto* item:scene()->selectedItems())if(item->data(0)=="part")return item->data(1).toString();return {};}
void CircuitCanvas::selectPart(const std::string& id){if(parts_.contains(id))parts_.at(id)->setSelected(true);}
void CircuitCanvas::mouseMoveEvent(QMouseEvent* e){if(panning_){const auto delta=e->pos()-panOrigin_;panOrigin_=e->pos();horizontalScrollBar()->setValue(horizontalScrollBar()->value()-delta.x());verticalScrollBar()->setValue(verticalScrollBar()->value()-delta.y());e->accept();return;}QGraphicsView::mouseMoveEvent(e);if(preview_&&wireStart_)preview_->setPath(wirePath(pinPosition(*wireStart_),mapToScene(e->pos())));}
void CircuitCanvas::mousePressEvent(QMouseEvent* e){if(e->button()==Qt::MiddleButton){panning_=true;panOrigin_=e->pos();viewport()->setCursor(Qt::ClosedHandCursor);e->accept();return;}QGraphicsView::mousePressEvent(e);}
void CircuitCanvas::mouseReleaseEvent(QMouseEvent* e){if(e->button()==Qt::MiddleButton&&panning_){panning_=false;viewport()->unsetCursor();e->accept();return;}QGraphicsView::mouseReleaseEvent(e);}
void CircuitCanvas::keyPressEvent(QKeyEvent* e){if(e->key()==Qt::Key_Escape){cancelWire();e->accept();}else if(e->key()==Qt::Key_Delete){removeSelected();e->accept();}else QGraphicsView::keyPressEvent(e);}
void CircuitCanvas::wheelEvent(QWheelEvent* e){if(e->modifiers()&Qt::ControlModifier){zoom(e->angleDelta().y()>0?1.15:1/1.15);e->accept();}else QGraphicsView::wheelEvent(e);}
void CircuitCanvas::drawBackground(QPainter* p,const QRectF& r){p->fillRect(r,QColor("#f7f9fc"));p->setPen(QPen(QColor("#dbe4ef"),1));for(double x=std::floor(r.left()/20)*20;x<r.right();x+=20)for(double y=std::floor(r.top()/20)*20;y<r.bottom();y+=20)p->drawPoint(QPointF(x,y));}
