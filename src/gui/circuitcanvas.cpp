#include "breadboardlayout.hpp"
#include "notelayout.hpp"
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsSimpleTextItem>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QJsonDocument>
#include <QJsonObject>
#include <stdexcept>
#include "language.hpp"
#include "projectio.hpp"
#include <QApplication>
#include <QClipboard>
#include <QMimeData>
#include <QMenu>
#include <QSignalBlocker>
#include <QInputDialog>
#include "componentdesign.hpp"
#include "motion.hpp"
#include <QStyleOptionGraphicsItem>
#include "circuitcanvas.hpp"
#include "thinkercad/diagnostics.hpp"
#include "wiringgeometry.hpp"
#include <QMessageBox>
#include "thinkercad/modules.hpp"
#include <QGraphicsEllipseItem>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QKeyEvent>
#include <QFocusEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QTimer>
#include <QScrollBar>
#include <QUndoCommand>
#include <QWheelEvent>
#include <cmath>
#include <utility>
using namespace thinkercad;
namespace {
QPainterPath wirePath(QPointF a,QPointF b) {QPainterPath p(a);p.lineTo(b);return p;}
QString partName(PartKind kind){return Language::text(QString::fromStdString(componentSpec(kind).label));}
bool switchable(PartKind kind){return kind==PartKind::InfraredRemote||kind==PartKind::NfcTestCard||kind==PartKind::ToggleSwitch||kind==PartKind::Button||kind==PartKind::SlideSwitch||kind==PartKind::TiltSwitch||kind==PartKind::ReedSwitch;}
double blockBoardScale(const CircuitPart& part){return part.kind==PartKind::Esp8266&&BoardProfile::get(part.model).arduino()?1.25:1.0;}
QRectF blockBounds(const CircuitPart& part){if(part.kind==PartKind::Breadboard30)return breadboardBody(part).adjusted(-3,-3,3,3);if(part.kind>=PartKind::Keypad)return ComponentDesign::bounds(part);const double scale=blockBoardScale(part);if(poweredBoard(part))return {0,0,503,330};if(part.kind==PartKind::Note)return noteBounds(part);return part.kind==PartKind::CharacterLcd||part.kind==PartKind::I2cCharacterLcd?QRectF(-10,-10,250,160):part.kind==PartKind::Junction?QRectF(-12,-12,24,24):part.kind==PartKind::NetLabel?QRectF(-8,-26,150,42):part.kind==PartKind::Esp8266?QRectF(-10*scale,-10*scale,230*scale,scale*std::max(370,202+static_cast<int>((CircuitDocument::pins(part).size()+1)/2-1)*24)):part.kind==PartKind::Oled?QRectF(-10,-10,310,224):part.kind==PartKind::Breadboard30?QRectF(0,0,570,260):QRectF(-10,-10,170,std::max(120,64+static_cast<int>((CircuitDocument::pins(part).size()+1)/2)*20));}
QPointF localPin(const CircuitPart& part,const std::string& pin) {if(part.kind==PartKind::Breadboard30)return breadboardPin(part,pin);if(ComponentDesign::realistic()||part.kind>=PartKind::Keypad)return ComponentDesign::pinPosition(part,pin);const auto kind=part.kind;if(poweredBoard(part))return railBoardPin(pin);
    if(kind==PartKind::CharacterLcd||kind==PartKind::I2cCharacterLcd){const auto names=CircuitDocument::pins(part);const auto i=std::find(names.begin(),names.end(),pin)-names.begin();return {10.+i*(kind==PartKind::CharacterLcd?14.:60.),140};}
    if(kind==PartKind::Junction||kind==PartKind::NetLabel)return {0,0};
    const auto names=CircuitDocument::pins(part);const int index=static_cast<int>(std::find(names.begin(),names.end(),pin)-names.begin());
    if(kind==PartKind::Esp8266){const int half=(names.size()+1)/2;return (index<half?QPointF(0,142+index*24):QPointF(210,142+(index-half)*24))*blockBoardScale(part);}
    if(kind==PartKind::Oled)return {47.0+index*66,200};
    if(kind==PartKind::Breadboard30){const int column=std::stoi(pin.substr(1))-1,row=pin[0]-'a';return {20.0+column*18,40.0+row*18+(row>=5?25:0)};}
    if(names.size()>2){const int half=(names.size()+1)/2;return {index<half?0.0:150.0,44.0+(index%half)*20};}
    return {index?150.0:0.0,64};
}
// Work in scene coordinates so rotations, the grid and the current design agree.
QPointF contactPosition(const CircuitPart& part,const std::string& pin){const auto center=(ComponentDesign::realistic()?ComponentDesign::bounds(part):blockBounds(part)).center();QTransform r;r.rotate(part.rotation);return QPointF(part.x,part.y)+center+r.map(localPin(part,pin)-center);}
void seatContacts(CircuitDocument& after,const CircuitDocument& before){
    std::set<std::string> moved;
    for(const auto& p:after.parts()){const auto old=std::find_if(before.parts().begin(),before.parts().end(),[&](const auto& q){return q.id==p.id;});if(old==before.parts().end()||old->x!=p.x||old->y!=p.y||old->rotation!=p.rotation)moved.insert(p.id);}
    if(moved.empty())return;
    struct Contact{Endpoint endpoint;QPointF position;};std::vector<Contact> fixed,loose;
    for(const auto& p:after.parts())for(const auto& pin:CircuitDocument::pins(p))(moved.contains(p.id)?loose:fixed).push_back({{p.id,pin},contactPosition(p,pin)});
    if(loose.empty())return;
    // Prefer a translation that seats more than one lead, then the smallest motion.
    std::map<std::pair<int,int>,std::vector<QPointF>> holes;
    for(const auto& b:fixed)holes[{int(std::floor(b.position.x())),int(std::floor(b.position.y()))}].push_back(b.position);
    auto seats=[&](QPointF p){const int x=std::floor(p.x()),y=std::floor(p.y());for(int dx=-1;dx<=1;++dx)for(int dy=-1;dy<=1;++dy){auto it=holes.find({x+dx,y+dy});if(it!=holes.end())for(auto q:it->second)if(QLineF(p,q).length()<.25)return true;}return false;};
    QPointF correction;int bestCount=0;double bestDistance=9;
    std::set<std::pair<int,int>> tried;
    for(const auto& a:loose)for(const auto& b:fixed){const auto delta=b.position-a.position;const auto distance=std::hypot(delta.x(),delta.y());if(distance>8||!tried.emplace(qRound(delta.x()*100),qRound(delta.y()*100)).second)continue;int count=0;
        for(const auto& lead:loose)if(seats(lead.position+delta))++count;
        if(count>bestCount||(count==bestCount&&distance<bestDistance)){bestCount=count;bestDistance=distance;correction=delta;}
    }
    if(bestCount)for(const auto& id:moved){after.part(id).x+=correction.x();after.part(id).y+=correction.y();}
    std::vector<std::string> detached;for(const auto& wire:after.wires())if(wire.direct&&(moved.contains(wire.a.part)||moved.contains(wire.b.part)))detached.push_back(wire.id);for(const auto& id:detached)after.removeWire(id);
    std::vector<Contact> contacts;for(const auto& p:after.parts())for(const auto& pin:CircuitDocument::pins(p))contacts.push_back({{p.id,pin},contactPosition(p,pin)});
    for(std::size_t i=0;i<contacts.size();++i)for(std::size_t j=i+1;j<contacts.size();++j){const auto& a=contacts[i];const auto& b=contacts[j];if(a.endpoint.part==b.endpoint.part||(!moved.contains(a.endpoint.part)&&!moved.contains(b.endpoint.part))||QLineF(a.position,b.position).length()>0.25)continue;
        if(std::none_of(after.wires().begin(),after.wires().end(),[&](const auto& w){return (w.a==a.endpoint&&w.b==b.endpoint)||(w.a==b.endpoint&&w.b==a.endpoint);})){after.connect(a.endpoint,b.endpoint);const_cast<CircuitWire&>(after.wires().back()).direct=true;}
    }
}
class PlacementPreview final:public QGraphicsItem {
public:
    CircuitPart part;std::set<std::string> seated;
    explicit PlacementPreview(CircuitPart p):part(std::move(p)){setData(0,"placementPreview");setAcceptedMouseButtons(Qt::NoButton);setZValue(10);setPos(part.x,part.y);}
    QRectF boundingRect()const override{return ComponentDesign::realistic()?ComponentDesign::bounds(part):blockBounds(part);}
    void paint(QPainter* p,const QStyleOptionGraphicsItem*,QWidget*)override{p->save();p->setOpacity(.6);if(ComponentDesign::realistic()||part.kind>=PartKind::Keypad)ComponentDesign::paint(*p,part,{},Frame{},1,false);else{p->setPen(QPen(QColor("#139ba4"),2,Qt::DashLine));p->setBrush(QColor("#c8e8e8"));p->drawRoundedRect(boundingRect(),5,5);p->drawText(boundingRect(),Qt::AlignCenter,partName(part.kind));}p->restore();for(const auto& pin:CircuitDocument::pins(part)){p->setPen(QPen(QColor(seated.contains(pin)?"#12c792":"#16a7c9"),2));p->setBrush(Qt::NoBrush);p->drawEllipse(localPin(part,pin),5,5);}}
};
class WireItem final:public QGraphicsPathItem {
public:
    WireItem(const CircuitWire& wire,const QPainterPath& path):QGraphicsPathItem(path){setData(0,"wire");setData(1,QString::fromStdString(wire.id));setFlags(ItemIsSelectable);setZValue(20);setPen(QPen(QColor(QString::fromStdString(wire.color)),2.8,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));setToolTip(QString::fromStdString(wire.a.part+"."+wire.a.pin+" → "+wire.b.part+"."+wire.b.pin));}
    QPainterPath shape() const override{QPainterPathStroker s;s.setWidth(14);return s.createStroke(path());}
    void paint(QPainter* p,const QStyleOptionGraphicsItem*,QWidget*)override{auto color=pen();if(isSelected()){color.setColor(QColor("#2563eb"));color.setWidthF(5);}p->setPen(color);p->setBrush(Qt::NoBrush);p->drawPath(path());}
};
class BendHandle final:public QGraphicsEllipseItem {
    CircuitCanvas* canvas_;std::string id_;std::size_t index_;
public:
    BendHandle(CircuitCanvas* canvas,const std::string& id,std::size_t index,QPointF point):QGraphicsEllipseItem(-6,-6,12,12),canvas_(canvas),id_(id),index_(index){setData(0,"bend");setData(1,QString::fromStdString(id));setData(2,static_cast<int>(index));setPos(point);setBrush(Qt::white);setPen(QPen(QColor("#0d9488"),2));setFlags(ItemIsMovable|ItemIgnoresTransformations);setZValue(22);setCursor(Qt::SizeAllCursor);setToolTip(Language::text("Kabelknick ziehen"));}
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event)override{QGraphicsEllipseItem::mouseMoveEvent(event);setPos(canvas_->constrainBend(id_,index_,pos(),event->modifiers()));}
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event)override{QGraphicsEllipseItem::mouseReleaseEvent(event);const auto point=pos();const auto modifiers=event->modifiers();QTimer::singleShot(0,canvas_,[c=canvas_,id=id_,i=index_,point,modifiers]{c->moveBend(id,i,point,modifiers);});}
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
        else if(endpoint_.pin=="GND"||endpoint_.pin=="GND2"||endpoint_.pin=="GND3")description=Language::text("Masse / Bezugspotential (0 V)");
        else if(endpoint_.pin=="SDA")description=Language::text("I²C-Datenleitung");
        else if(endpoint_.pin=="SCL")description=Language::text("I²C-Taktleitung");
        else if(endpoint_.pin=="VCC"||endpoint_.pin=="3V3"||endpoint_.pin=="3V3_2"||endpoint_.pin=="5V")description=Language::text("Versorgungsspannung");
        else if(endpoint_.pin=="W")description=Language::text("Schleifer");
        else if((endpoint_.pin=="COM"||endpoint_.pin=="COM2"))description=Language::text("Gemeinsame Kathode");
        else if(endpoint_.pin=="+"||endpoint_.pin=="-")description=Language::text(endpoint_.pin=="+"?"Pluspol":"Minuspol");
        else if(part.kind==PartKind::Esp8266)description=Language::text("GPIO / Boardanschluss");
        else description=Language::text("Anschluss")+" "+pin;
        setToolTip(pin+" — "+description+"\n"+QString::fromStdString(componentSpec(part.kind).label)+" · "+QString::fromStdString(part.id)+"\n"+Language::text("Zum Verbinden klicken"));}
    void paint(QPainter* painter,const QStyleOptionGraphicsItem* option,QWidget* widget)override{
        if(!realistic_){QGraphicsEllipseItem::paint(painter,option,widget);if(data(3).toBool()){painter->setPen(QPen(QColor("#14b8a6"),2));painter->setBrush(Qt::NoBrush);painter->drawEllipse(rect().adjusted(-2,-2,2,2));}return;}
        const auto kind=canvas_->document().part(endpoint_.part).kind;
        const bool board=kind==PartKind::Esp8266||kind==PartKind::Oled;
        if(kind==PartKind::Breadboard30){painter->setPen(QPen(QColor("#b7b9b4"),.7));painter->setBrush(QColor("#c5c7c2"));painter->drawEllipse(QRectF(-2.8,-2.8,5.6,5.6));painter->setPen(Qt::NoPen);painter->setBrush(QColor("#3f4240"));painter->drawEllipse(QRectF(-1.5,-1.5,3,3));if((option->state&QStyle::State_MouseOver)||data(3).toBool()){painter->setPen(QPen(QColor("#14b8a6"),1));painter->setBrush(Qt::NoBrush);painter->drawEllipse(QRectF(-5,-5,10,10));}return;}
        if(kind==PartKind::Led||kind==PartKind::InfraredLed||kind==PartKind::Button||kind==PartKind::Resistor){if(option->state&QStyle::State_MouseOver){painter->setPen(QPen(QColor("#14b8a6"),1));painter->setBrush(Qt::NoBrush);painter->drawEllipse(QRectF(-4,-4,8,8));}return;}
        painter->setPen(QPen(board?QColor("#c7ad6d"):QColor("#929da4"),1.2));
        painter->setBrush(board?QColor("#c7ad6d"):QColor("#d4d9da"));painter->drawRoundedRect(QRectF(-4,-4,8,8),1,1);
        painter->setPen(Qt::NoPen);const bool seated=std::any_of(canvas_->document().wires().begin(),canvas_->document().wires().end(),[&](const auto& w){return w.direct&&(w.a==endpoint_||w.b==endpoint_);});painter->setBrush(QColor(seated?"#16c9a8":"#202830"));painter->drawEllipse(QRectF(-2,-2,4,4));
        if((option->state&QStyle::State_MouseOver)||data(3).toBool()){painter->setPen(QPen(QColor("#14b8a6"),2));painter->setBrush(Qt::NoBrush);painter->drawEllipse(QRectF(-6,-6,12,12));}
    }
    void hoverEnterEvent(QGraphicsSceneHoverEvent* e)override{canvas_->previewNet(endpoint_);QGraphicsEllipseItem::hoverEnterEvent(e);}
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* e)override{canvas_->clearNetPreview();QGraphicsEllipseItem::hoverLeaveEvent(e);}
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
    Qt::Edges resizeEdges_;
    QRectF resizeStart_;
    QPointF resizePointer_;
    bool noteHover_=false;
    Qt::Edges edgesAt(QPointF point)const{
        const auto transform=canvas_->viewportTransform();
        const double tolerance=std::min({bounds_.width()/4,bounds_.height()/4,6./std::max(.05,std::hypot(transform.m11(),transform.m12()))});
        Qt::Edges edges;
        if(std::abs(point.x()-bounds_.left())<=tolerance)edges|=Qt::LeftEdge;
        else if(std::abs(point.x()-bounds_.right())<=tolerance)edges|=Qt::RightEdge;
        if(std::abs(point.y()-bounds_.top())<=tolerance)edges|=Qt::TopEdge;
        else if(std::abs(point.y()-bounds_.bottom())<=tolerance)edges|=Qt::BottomEdge;
        return edges;
    }
    void noteCursor(QPointF point){
        auto edges=edgesAt(point);bool horizontal=edges.testFlag(Qt::LeftEdge)||edges.testFlag(Qt::RightEdge),vertical=edges.testFlag(Qt::TopEdge)||edges.testFlag(Qt::BottomEdge);
        const bool quarterTurn=int(rotation())%180!=0;
        if(horizontal&&vertical){bool forward=(edges.testFlag(Qt::LeftEdge)&&edges.testFlag(Qt::TopEdge))||(edges.testFlag(Qt::RightEdge)&&edges.testFlag(Qt::BottomEdge));setCursor(forward!=quarterTurn?Qt::SizeFDiagCursor:Qt::SizeBDiagCursor);}
        else if(horizontal||vertical)setCursor(horizontal!=quarterTurn?Qt::SizeHorCursor:Qt::SizeVerCursor);
        else setCursor(Qt::OpenHandCursor);
    }
    void hoverEnterEvent(QGraphicsSceneHoverEvent* event)override{if(kind==PartKind::Note&&canvas_->editingEnabled()){noteHover_=true;noteCursor(event->pos());update();}QGraphicsItem::hoverEnterEvent(event);}
    void hoverMoveEvent(QGraphicsSceneHoverEvent* event)override{if(kind==PartKind::Note&&canvas_->editingEnabled()){noteHover_=true;noteCursor(event->pos());update();}QGraphicsItem::hoverMoveEvent(event);}
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event)override{noteHover_=false;if(kind==PartKind::Note)setCursor(canvas_->editingEnabled()?Qt::OpenHandCursor:Qt::ArrowCursor);update();QGraphicsItem::hoverLeaveEvent(event);}

public:
    std::string id;PartKind kind;CircuitState state;Frame visible;double brightness=1;
    PartItem(CircuitCanvas* c,const CircuitPart& p):canvas_(c),bounds_((realistic_&&p.kind!=PartKind::Note)||p.kind==PartKind::Breadboard30?ComponentDesign::bounds(p):blockBounds(p)),id(p.id),kind(p.kind){setData(0,"part");setData(1,QString::fromStdString(id));setFlags(ItemIsSelectable|ItemIsMovable|ItemSendsGeometryChanges);setZValue(p.kind==PartKind::Breadboard30?-5:1);setPos(p.x,p.y);setTransformOriginPoint(boundingRect().center());setRotation(p.rotation);initializing_=false;setCursor(Qt::OpenHandCursor);if(kind==PartKind::Note){setToolTip(QString::fromStdString(p.model));setAcceptHoverEvents(true);}for(const auto& pin:CircuitDocument::pins(p)){auto point=localPin(p,pin);auto* port=new PinItem(c,{id,pin},this);port->setPos(point);}}

    QRectF boundingRect()const override{return bounds_;}
    QVariant itemChange(GraphicsItemChange change,const QVariant& v)override{
        if(change==ItemPositionChange&&!initializing_){auto pos=v.toPointF();pos=canvas_->snapped(pos);return QPointF(std::clamp(pos.x(),-9500.0,9500.0),std::clamp(pos.y(),-9500.0,9500.0));}
        if(change==ItemPositionHasChanged&&!initializing_){try{auto& p=const_cast<CircuitDocument&>(canvas_->document()).part(id);p.x=pos().x();p.y=pos().y();}catch(const std::exception&){} }
        return QGraphicsItem::itemChange(change,v);
    }
    void mousePressEvent(QGraphicsSceneMouseEvent* e)override{
        before_=canvas_->document();
        if(kind==PartKind::Note&&canvas_->editingEnabled()&&e->button()==Qt::LeftButton){resizeEdges_=edgesAt(e->pos());if(resizeEdges_){resizeStart_=bounds_;resizePointer_=e->pos();setSelected(true);e->accept();return;}}
        QGraphicsItem::mousePressEvent(e);
    }
    void mouseMoveEvent(QGraphicsSceneMouseEvent* e)override{
        if(resizeEdges_){if(canvas_->editingEnabled()){
            auto rect=resizeStart_;const auto delta=mapFromScene(e->scenePos())-resizePointer_;
            if(resizeEdges_.testFlag(Qt::LeftEdge))rect.setLeft(std::clamp(rect.left()+delta.x(),rect.right()-4096,rect.right()-32));
            if(resizeEdges_.testFlag(Qt::RightEdge))rect.setRight(std::clamp(rect.right()+delta.x(),rect.left()+32,rect.left()+4096));
            if(resizeEdges_.testFlag(Qt::TopEdge))rect.setTop(std::clamp(rect.top()+delta.y(),rect.bottom()-4096,rect.bottom()-32));
            if(resizeEdges_.testFlag(Qt::BottomEdge))rect.setBottom(std::clamp(rect.bottom()+delta.y(),rect.top()+32,rect.top()+4096));
            prepareGeometryChange();bounds_=rect;update();
        }e->accept();return;}QGraphicsItem::mouseMoveEvent(e);
    }
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* e)override{
        if(resizeEdges_){resizeEdges_={};const auto rect=bounds_;
            if(canvas_->editingEnabled()&&rect!=resizeStart_){
                const auto corner=mapToScene(rect.topLeft()),center=QPointF(rect.width()/2,rect.height()/2);QTransform rotate;rotate.rotate(rotation());const auto position=corner-center+rotate.map(center);
                QTimer::singleShot(0,canvas_,[c=canvas_,partId=id,position,size=rect.size()]{if(!c->editingEnabled())return;c->mutate(Language::text("Notiz bearbeiten"),[&](auto& doc){auto& part=doc.part(partId);part.x=position.x();part.y=position.y();part.parameters["noteWidth"]=size.width();part.parameters["noteHeight"]=size.height();});});
            }
            // Restore the preview before the deferred transaction; failed or cancelled
            // edits must leave the item consistent with the saved document.
            prepareGeometryChange();bounds_=blockBounds(canvas_->document().part(id));update();
            e->accept();return;
        }
        QGraphicsItem::mouseReleaseEvent(e);if(canvas_->editingEnabled()){const auto& a=before_.part(id);const auto& b=canvas_->document().part(id);if(a.x!=b.x||a.y!=b.y){const auto old=before_;QTimer::singleShot(0,canvas_,[c=canvas_,old]{c->recordMove(old);});}}}
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* e)override{if(kind>=PartKind::Keypad){emit canvas_->componentInputRequested(QString::fromStdString(id));e->accept();return;}if(kind==PartKind::Note&&canvas_->editingEnabled()){canvas_->editNote(id);e->accept();return;}if(switchable(kind)){canvas_->toggleButton(id);e->accept();}else QGraphicsItem::mouseDoubleClickEvent(e);}
    void paint(QPainter* p,const QStyleOptionGraphicsItem*,QWidget*)override{
        const auto& part=canvas_->document().part(id);if(part.kind>=PartKind::Keypad){ComponentDesign::paint(*p,part,state,visible,brightness,isSelected());return;}p->setRenderHint(QPainter::Antialiasing);
        if(kind==PartKind::Note){p->setPen(QPen(isSelected()?QColor("#168a9e"):QColor("#cfb866"),isSelected()?2:1));p->setBrush(QColor("#fff2b9"));p->drawRoundedRect(bounds_.adjusted(1,1,-1,-1),4,4);p->save();p->setClipRect(bounds_.adjusted(8,8,-8,-8));p->setPen(QColor("#4e431e"));p->setFont(noteFont());p->drawText(bounds_.adjusted(8,8,-8,-8),Qt::TextWordWrap|Qt::TextWrapAnywhere|Qt::AlignLeft|Qt::AlignTop,noteText(part));p->restore();
            if(canvas_->editingEnabled()&&(isSelected()||noteHover_)){p->setPen(QPen(QColor("#168a9e"),1));p->setBrush(Qt::white);const auto r=bounds_.adjusted(3,3,-3,-3);for(auto point:{r.topLeft(),r.topRight(),r.bottomLeft(),r.bottomRight(),QPointF(r.center().x(),r.top()),QPointF(r.center().x(),r.bottom()),QPointF(r.left(),r.center().y()),QPointF(r.right(),r.center().y())})p->drawRect(QRectF(point-QPointF(2,2),QSizeF(4,4)));}return;}
        if(realistic_&&(kind==PartKind::CharacterLcd||kind==PartKind::I2cCharacterLcd)){ComponentDesign::paint(*p,part,state,visible,brightness,isSelected());return;}
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
            p->setPen(QColor("#d7eeea"));p->setFont(QFont("Segoe UI",8));for(int i=0;i<static_cast<int>(names.size());++i){auto label=QString::fromStdString(names[i]);if(label.startsWith("GPIO"))label.replace("GPIO","IO ");if(profile.supportsPwm(profile.pinNumber(names[i])))label.prepend("~");p->drawText(QRectF(i<half?12:135,134+(i%half)*24,64,20),i<half?Qt::AlignLeft:Qt::AlignRight,label);}
            p->setPen(Qt::NoPen);p->setBrush(state.level({id,profile.pinName(profile.builtinLed)})==(profile.arduino()?Level::High:Level::Low)?QColor("#4dd7ff"):QColor("#245f6b"));p->drawEllipse(QPointF(105,242),5,5);p->setBrush(QColor("#a5b6bf"));p->drawRoundedRect(86,boardHeight-21,38,23,3,3);
        }else if(kind==PartKind::Oled){
            p->setBrush(QColor("#173d5b"));p->drawRoundedRect(0,0,292,204,12,12);p->setBrush(QColor("#030d17"));p->setPen(Qt::NoPen);p->drawRoundedRect(18,18,256,128,4,4);
            const auto v=state.voltage.find({id,"VCC"}),g=state.voltage.find({id,"GND"});const bool powered=v!=state.voltage.end()&&g!=state.voltage.end()&&v->second&&g->second&&*v->second-*g->second>=3.0;
            if(powered){auto light=QColor("#8df0eb");light.setAlphaF(brightness);p->setBrush(light);for(int y=0;y<visible.height;++y)for(int x=0;x<128;++x)if(visible.pixel(x,y))p->drawRect(QRectF(18+x*2,18+y*2,1.8,1.8));}
            p->setPen(QColor("#c3e6ef"));p->setFont(QFont("Segoe UI",9));p->drawText(QRectF(12,152,268,24),Qt::AlignCenter,QString("SSD1306 · 128 × %2 · 0x%1").arg(static_cast<int>(part.value),2,16,QChar('0')).arg(CircuitDocument::displayHeight(part)).toUpper());const QStringList pins{"GND","VCC","SCL","SDA"};p->setFont(QFont("Segoe UI",8));for(int i=0;i<4;++i)p->drawText(QRectF(27+i*66,176,42,18),Qt::AlignCenter,pins[i]);
        }else if(kind==PartKind::Breadboard30){paintBreadboard(*p,part);
        }else if(kind==PartKind::Breadboard30){p->setBrush(QColor("#e8edf0"));p->drawRoundedRect(0,0,570,260,12,12);p->setPen(QColor("#475569"));p->setFont(QFont("Segoe UI",7));for(int n=1;n<=30;++n)p->drawText(QRectF(12+(n-1)*18,10,18,20),Qt::AlignCenter,QString::number(n));p->drawText(QRectF(10,226,550,24),Qt::AlignCenter,Language::text("a–e / f–j: je fünf Kontakte pro Spalte verbunden · keine Stromschienen"));
        }else{
            p->drawRoundedRect(0,0,150,boundingRect().height()-16,12,12);p->setPen(QColor("#334155"));p->setFont(QFont("Segoe UI",10,QFont::DemiBold));p->drawText(QRectF(8,8,134,22),Qt::AlignCenter,kind==PartKind::Module?QString::fromStdString(ModuleRegistry::get(part.model).label):partName(kind));
            if(kind==PartKind::Led||kind==PartKind::InfraredLed){const auto current=state.ledCurrent.contains(id)?state.ledCurrent.at(id):0;p->setPen(Qt::NoPen);p->setBrush(current>1e-5?QColor(QString::fromStdString(part.color)):QColor("#b6bfc9"));if(current>1e-5){auto glow=QColor(QString::fromStdString(part.color));glow.setAlpha(35);p->setBrush(glow);p->drawEllipse(QPointF(75,58),27,27);p->setBrush(QColor(QString::fromStdString(part.color)));}p->drawEllipse(QPointF(75,58),17,17);p->setPen(QColor("#64748b"));p->setFont(QFont("Segoe UI",8));p->drawText(QRectF(8,81,134,18),Qt::AlignCenter,QString("%1 mA · A → K").arg(current*1000,0,'f',1));}
            if(kind==PartKind::Resistor){p->setBrush(QColor("#e7c99b"));p->setPen(Qt::NoPen);p->drawRoundedRect(30,45,90,25,5,5);p->setBrush(QColor("#926a40"));for(int x:{48,61,83,102})p->drawRect(x,45,5,25);p->setPen(QColor("#64748b"));p->setFont(QFont("Segoe UI",8));p->drawText(QRectF(8,81,134,18),Qt::AlignCenter,QString("%1 Ω").arg(part.value));}
            if(switchable(kind)){p->setBrush(part.pressed?QColor("#0d9488"):QColor("#e2e8f0"));p->drawRoundedRect(50,39,50,34,8,8);p->setPen(QColor("#64748b"));p->setFont(QFont("Segoe UI",8));p->drawText(QRectF(8,81,134,18),Qt::AlignCenter,part.pressed?Language::text("Gedrückt"):Language::text("Zum Betätigen gedrückt halten"));}
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
CircuitCanvas::CircuitCanvas(CircuitDocument& doc,QWidget* parent):QGraphicsView(parent),document_(doc),history_(this){setScene(new QGraphicsScene(this));setAcceptDrops(true);setRenderHints(QPainter::Antialiasing|QPainter::SmoothPixmapTransform);setViewportUpdateMode(BoundingRectViewportUpdate);setDragMode(RubberBandDrag);setTransformationAnchor(AnchorUnderMouse);setMinimumSize(180,180);setFrameShape(QFrame::NoFrame);setObjectName("circuitCanvas");connect(scene(),&QGraphicsScene::selectionChanged,this,[this]{emit selectionChanged();QTimer::singleShot(0,this,&CircuitCanvas::refreshBendHandles);});connect(scene(),&QGraphicsScene::changed,this,[this]{redrawWires();});rebuild();}
CircuitCanvas::~CircuitCanvas(){scene()->disconnect(this);}
void CircuitCanvas::resetHistory(){history_.clear();history_.setClean();}
void CircuitCanvas::mutate(const QString& label,std::function<void(CircuitDocument&)> action){if(!editingEnabled_){emit notice(Language::text("Simulation zuerst stoppen, um die Schaltung zu ändern."));return;}try{auto after=document_;action(after);seatContacts(after,document_);after.validate();history_.push(new Snapshot(this,document_,std::move(after),label));}catch(const std::exception& e){emit notice(QString::fromUtf8(e.what()));}}
void CircuitCanvas::applyDocument(const CircuitDocument& doc){
    std::set<std::string> oldParts,oldWires;for(const auto& p:document_.parts())oldParts.insert(p.id);for(const auto& w:document_.wires())oldWires.insert(w.id);
    document_=doc;rebuild();
    if(Motion::enabled()){
        std::vector<QGraphicsItem*> targets;
        for(const auto& [id,item]:parts_)if(!oldParts.contains(id))targets.push_back(item);
        for(auto* item:scene()->items())if(item->data(0)=="wire"&&!oldWires.contains(item->data(1).toString().toStdString()))targets.push_back(item);
        if(!targets.empty()){auto* animation=new QVariantAnimation(this);animation->setObjectName("assemblyMotion");animation->setDuration(160);animation->setStartValue(.55);animation->setEndValue(1.);animation->setEasingCurve(QEasingCurve::OutCubic);for(auto* item:targets)item->setOpacity(.55);connect(animation,&QVariantAnimation::valueChanged,this,[targets](const QVariant& v){for(auto* item:targets)item->setOpacity(v.toDouble());});connect(animation,&QVariantAnimation::finished,animation,&QObject::deleteLater);animation->start();}
        for(const auto& wire:document_.wires())if(wire.direct&&!oldWires.contains(wire.id)){auto* ring=scene()->addEllipse(QRectF(pinPosition(wire.a)-QPointF(7,7),QSizeF(14,14)),QPen(QColor("#18ae98"),1.8));ring->setZValue(23);ring->setAcceptedMouseButtons(Qt::NoButton);auto* animation=new QVariantAnimation(this);animation->setObjectName("assemblyMotion");animation->setDuration(200);animation->setStartValue(1.);animation->setEndValue(0.);connect(animation,&QVariantAnimation::valueChanged,this,[ring](const QVariant& v){ring->setOpacity(v.toDouble());});connect(animation,&QVariantAnimation::finished,this,[ring,animation]{delete ring;animation->deleteLater();});animation->start();}
    }
    emit documentChanged();
}
void CircuitCanvas::rebuild(){
    netOverlays_.clear();const auto signature=NetTopology::signature(document_);if(signature!=topologySignature_){topology_.reset();topologySignature_=signature;}

    for(auto* animation:findChildren<QVariantAnimation*>("assemblyMotion")){animation->stop();delete animation;}
    QSignalBlocker sceneSignals(scene());clearPlacementPreview();cancelWire();
    std::set<std::string> selectedParts,selectedWires;
    for(auto* item:scene()->selectedItems()){
        const auto id=item->data(1).toString().toStdString();
        if(item->data(0)=="part")selectedParts.insert(id);
        if(item->data(0)=="wire")selectedWires.insert(id);
    }
    scene()->clear();parts_.clear();
    for(const auto& part:document_.parts()){
        auto* item=new PartItem(this,part);
        item->setFlag(QGraphicsItem::ItemIsMovable,editingEnabled_&&!wireMode_);item->setFlag(QGraphicsItem::ItemIsSelectable,editingEnabled_);
        scene()->addItem(item);parts_[part.id]=item;
        item->setSelected(selectedParts.contains(part.id));
    }
    for(const auto& wire:document_.wires()){
        if(wire.direct)continue;
        auto* item=new WireItem(wire,routedWire(wire));item->setFlag(QGraphicsItem::ItemIsSelectable,editingEnabled_);scene()->addItem(item);
        item->setSelected(selectedWires.contains(wire.id));
    }
    scene()->setSceneRect(scene()->itemsBoundingRect().adjusted(-500,-350,600,400));
    setEditingEnabled(editingEnabled_);viewport()->update();sceneSignals.unblock();emit selectionChanged();
}
QPainterPath CircuitCanvas::routedWire(const CircuitWire& wire) const {
    QPainterPath path(pinPosition(wire.a));
    for(const auto& point:wire.points)path.lineTo(point.x,point.y);
    path.lineTo(pinPosition(wire.b));return path;
}

void CircuitCanvas::redrawWires(){for(auto* item:scene()->items())if(item->data(0)=="wire"){const auto id=item->data(1).toString().toStdString();for(const auto& w:document_.wires())if(w.id==id){auto* path=static_cast<QGraphicsPathItem*>(item);auto rendered=w;for(auto* handle:scene()->items())if(handle->data(0)=="bend"&&handle->data(1).toString().toStdString()==w.id){const auto i=handle->data(2).toInt();if(i>=0&&i<static_cast<int>(rendered.points.size()))rendered.points[i]={handle->pos().x(),handle->pos().y()};}const auto next=routedWire(rendered);if(next!=path->path())path->setPath(next);break;}}}
void CircuitCanvas::setEditingEnabled(bool enabled){
    if(enabled&&!heldButton_.empty()){const auto id=std::exchange(heldButton_,{});setButtonPressed(id,false);}
    editingEnabled_=enabled;if(!enabled)clearPlacementPreview();cancelWire();
    if(!enabled)scene()->clearSelection();
    setDragMode(enabled&&!wireMode_?RubberBandDrag:NoDrag);
    for(auto* item:scene()->items()){
        if(item->data(0)=="part"||item->data(0)=="wire")item->setFlag(QGraphicsItem::ItemIsSelectable,enabled);
        if(auto* pin=dynamic_cast<PinItem*>(item)){
            pin->setCursor(enabled?Qt::CrossCursor:Qt::ArrowCursor);
            auto tooltip=pin->toolTip();const auto line=tooltip.lastIndexOf('\n');if(line>=0)tooltip=tooltip.left(line+1);
            pin->setToolTip(tooltip+Language::text(enabled?"Zum Verbinden klicken":"Simulation zuerst stoppen, um Kabel zu ändern."));
        }
    }
    for(auto& [id,item]:parts_){item->setFlag(QGraphicsItem::ItemIsMovable,enabled&&!wireMode_);item->setCursor(enabled?Qt::OpenHandCursor:switchable(document_.part(id).kind)?Qt::PointingHandCursor:Qt::ArrowCursor);}
    history_.setActive(enabled);refreshBendHandles();
}
void CircuitCanvas::setWireMode(bool enabled){wireMode_=enabled;cancelWire();setDragMode(enabled||!editingEnabled_?NoDrag:RubberBandDrag);for(auto& [id,item]:parts_){(void)id;item->setFlag(QGraphicsItem::ItemIsMovable,editingEnabled_&&!enabled);}}
QPointF CircuitCanvas::pinPosition(const Endpoint& ep)const{return parts_.at(ep.part)->mapToScene(localPin(document_.part(ep.part),ep.pin));}
void CircuitCanvas::connectPin(const Endpoint& pin){if(!editingEnabled_){emit notice(Language::text("Simulation zuerst stoppen, um Kabel zu ändern."));return;}if(!wireStart_){wireStart_=pin;preview_=scene()->addPath(wirePath(pinPosition(pin),pinPosition(pin)),QPen(wireColor_,2,Qt::DashLine));preview_->setZValue(21);emit notice(Language::text("Zielpin anklicken. Esc bricht das Kabel ab."));return;}const auto a=*wireStart_;const auto bends=pendingBends_;cancelWire();if(a==pin)return;
    for(const auto& w:document_.wires())if((w.a==a&&w.b==pin)||(w.b==a&&w.a==pin)){emit notice(Language::text("Dieses Kabel ist bereits vorhanden."));return;}
    auto candidate=document_;candidate.connect(a,pin);std::map<std::string,Esp8266Board> idle;std::map<std::string,const Esp8266Board*> boards;for(const auto& part:candidate.parts())if(part.kind==PartKind::Esp8266){idle[part.id].configure(part.model);boards[part.id]=&idle.at(part.id);}
    const auto issues=DiagnosticEngine::analyze(candidate,NetTopology(candidate),{},boards);if(std::any_of(issues.begin(),issues.end(),[](const auto& issue){return issue.rule=="driver-conflict"||issue.rule=="source-short";})&&QMessageBox::warning(this,Language::text("Elektrischer Konflikt"),Language::text("Diese Verbindung verbindet widersprüchliche Versorgungen. Trotzdem verbinden?"),QMessageBox::Yes|QMessageBox::No,QMessageBox::No)!=QMessageBox::Yes)return;
std::vector<QPointF> candidateRoute{pinPosition(a)};for(const auto& bend:bends)candidateRoute.emplace_back(bend.x,bend.y);candidateRoute.push_back(pinPosition(pin));bool overlap=false;for(const auto& existing:document_.wires()){if(existing.direct)continue;std::vector<QPointF> route{pinPosition(existing.a)};for(const auto& bend:existing.points)route.emplace_back(bend.x,bend.y);route.push_back(pinPosition(existing.b));for(std::size_t i=1;i<candidateRoute.size()&&!overlap;++i)for(std::size_t j=1;j<route.size();++j)if(overlappingSegments(QLineF(candidateRoute[i-1],candidateRoute[i]),QLineF(route[j-1],route[j]))){overlap=true;break;}}if(overlap&&QMessageBox::warning(this,Language::text("Überlappende Kabel"),Language::text("Kabel überdecken sich teilweise. Eine Überdeckung verbindet keine Netze. Trotzdem verlegen?"),QMessageBox::Yes|QMessageBox::No,QMessageBox::No)!=QMessageBox::Yes)return;
    const auto color=wireColor_.name().toStdString();QTimer::singleShot(0,this,[this,a,pin,color,bends]{mutate(Language::text("Kabel verbinden"),[&](auto& doc){doc.connect(a,pin,color);const_cast<CircuitWire&>(doc.wires().back()).points=bends;});});}
void CircuitCanvas::cancelWire(){clearNetPreview();wireStart_.reset();pendingBends_.clear();if(preview_){scene()->removeItem(preview_);delete preview_;preview_=nullptr;}}
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
void CircuitCanvas::recordMove(const CircuitDocument& before){seatContacts(document_,before);history_.push(new Snapshot(this,before,document_,Language::text("Bauteil verschieben")));}
void CircuitCanvas::setButtonPressed(const std::string& id,bool pressed){
    if(document_.part(id).pressed==pressed)return;
    if(editingEnabled_){mutate(Language::text("Kontakt umschalten"),[&](auto& doc){doc.part(id).pressed=pressed;});return;}
    document_.part(id).pressed=pressed;if(parts_.contains(id))parts_.at(id)->update();if(document_.part(id).kind!=PartKind::Button&&document_.part(id).kind!=PartKind::InfraredRemote)emit inputValueChanged();emit runtimeInputChanged();
}
void CircuitCanvas::toggleButton(const std::string& id){setButtonPressed(id,!document_.part(id).pressed);}
void CircuitCanvas::focusOutEvent(QFocusEvent* event){
    if(!heldButton_.empty()){const auto id=std::exchange(heldButton_,{});setButtonPressed(id,false);}
    QGraphicsView::focusOutEvent(event);
}
void CircuitCanvas::fitCircuit(){fitInView(scene()->itemsBoundingRect().adjusted(-50,-50,50,50),Qt::KeepAspectRatio);if(transform().m11()>1.4){resetTransform();scale(1.4,1.4);}}
void CircuitCanvas::zoom(double factor){const auto target=transform().m11()*factor;if(target>=0.25&&target<=3.5)scale(factor,factor);}
void CircuitCanvas::updateSimulation(const CircuitState& state,const std::map<std::string,OledDisplay>& displays){for(auto& [id,item]:parts_){item->state=state;if(displays.contains(id)){item->visible=displays.at(id).frame();item->brightness=displays.at(id).brightness();}}viewport()->update();}
QString CircuitCanvas::selectedPart()const{for(auto* item:scene()->selectedItems())if(item->data(0)=="part")return item->data(1).toString();return {};}
void CircuitCanvas::selectPart(const std::string& id){scene()->clearSelection();if(parts_.contains(id))parts_.at(id)->setSelected(true);}
void CircuitCanvas::mouseMoveEvent(QMouseEvent* e){if(panning_){const auto delta=e->pos()-panOrigin_;panOrigin_=e->pos();horizontalScrollBar()->setValue(horizontalScrollBar()->value()-delta.x());verticalScrollBar()->setValue(verticalScrollBar()->value()-delta.y());e->accept();return;}QGraphicsView::mouseMoveEvent(e);lastPointer_=mapToScene(e->pos());if(preview_&&wireStart_)preview_->setPath(previewPath(wirePoint(lastPointer_,e->modifiers())));}
void CircuitCanvas::mousePressEvent(QMouseEvent* e){
    if(editingEnabled_&&e->button()==Qt::LeftButton)for(auto* item:items(e->pos()))if(auto* pin=dynamic_cast<PinItem*>(item)){connectPin(pin->endpoint());e->accept();return;}

    if(!editingEnabled_&&e->button()!=Qt::MiddleButton){
        QGraphicsItem* hit=nullptr;for(auto* candidate:items(e->pos()))if(dynamic_cast<PartItem*>(candidate)){hit=candidate;break;}
        if(e->button()==Qt::LeftButton){
            while(hit&&hit->parentItem())hit=hit->parentItem();
            if(auto* control=dynamic_cast<PartItem*>(hit);control&&(control->kind>=PartKind::Keypad||control->kind==PartKind::Potentiometer||control->kind==PartKind::AnalogSource||control->kind==PartKind::Photoresistor||control->kind==PartKind::Thermistor||control->kind==PartKind::TemperatureSensor||control->kind==PartKind::FunctionGenerator||control->kind==PartKind::PowerSupply)){emit componentInputRequested(QString::fromStdString(control->id));e->accept();return;}
            if(auto* part=dynamic_cast<PartItem*>(hit);part&&switchable(part->kind)){
                if(part->kind==PartKind::Button||part->kind==PartKind::InfraredRemote){heldButton_=part->id;setButtonPressed(part->id,true);}
                else toggleButton(part->id);
            }
        }
        e->accept();return;
    }
if(e->button()==Qt::RightButton){auto* item=itemAt(e->pos());QMenu menu(this);if(auto* pin=dynamic_cast<PinItem*>(item)){auto ep=pin->endpoint();menu.addAction(Language::text("Netz hervorheben"),this,[this,ep]{highlightNet(ep);});if(editingEnabled_)menu.addAction(Language::text("Netzlabel setzen…"),this,[this,ep]{bool ok=false;auto name=QInputDialog::getText(this,Language::text("Netzlabel"),Language::text("Netzname (gleicher Name verbindet Pins)"),QLineEdit::Normal,{},&ok);if(ok&&!name.isEmpty()){auto point=pinPosition(ep)+QPointF(30,-30);mutate(Language::text("Netzlabel setzen"),[&](auto& doc){auto id=doc.addPart(PartKind::NetLabel,point.x(),point.y());doc.part(id).netName=name.toStdString();doc.connect(ep,{id,"NET"});});}});}else if(item&&item->data(0)=="wire"&&editingEnabled_){auto id=item->data(1).toString().toStdString();auto pos=mapToScene(e->pos());menu.addAction(Language::text("Abzweigung erstellen"),this,[this,id,pos]{branchWire(id,pos);});}if(editingEnabled_&&!item){const auto position=snapped(mapToScene(e->pos()));menu.addAction(Language::text("Notiz hinzufügen…"),this,[this,position]{bool ok=false;auto text=QInputDialog::getMultiLineText(this,Language::text("Notiz hinzufügen"),Language::text("Text"),{},&ok);if(ok&&!text.trimmed().isEmpty())mutate(Language::text("Notiz hinzufügen"),[&](auto& doc){auto id=doc.addPart(PartKind::Note,position.x(),position.y());doc.part(id).model=text.toStdString();});});}else if(editingEnabled_&&item&&item->data(0)=="part"&&document_.part(item->data(1).toString().toStdString()).kind==PartKind::Note){const auto id=item->data(1).toString().toStdString();menu.addAction(Language::text("Notiz bearbeiten…"),this,[this,id]{editNote(id);});}if(!menu.isEmpty()){menu.exec(e->globalPosition().toPoint());e->accept();return;}}
if(e->button()==Qt::LeftButton){for(auto* candidate:items(e->pos()))if(auto* pin=dynamic_cast<PinItem*>(candidate)){connectPin(pin->endpoint());e->accept();return;}}if(e->button()==Qt::LeftButton&&wireStart_&&!dynamic_cast<PinItem*>(itemAt(e->pos()))){if(pendingBends_.size()<64){auto point=wirePoint(mapToScene(e->pos()),e->modifiers());pendingBends_.push_back({point.x(),point.y()});if(preview_)preview_->setPath(previewPath(point));}e->accept();return;}if(e->button()==Qt::MiddleButton){panning_=true;panOrigin_=e->pos();viewport()->setCursor(Qt::ClosedHandCursor);e->accept();return;}QGraphicsView::mousePressEvent(e);}
void CircuitCanvas::mouseReleaseEvent(QMouseEvent* e){
    if(e->button()==Qt::LeftButton&&!heldButton_.empty()){const auto id=std::exchange(heldButton_,{});setButtonPressed(id,false);e->accept();return;}
    if(!editingEnabled_&&e->button()!=Qt::MiddleButton){e->accept();return;}
if(e->button()==Qt::MiddleButton&&panning_){panning_=false;viewport()->unsetCursor();e->accept();return;}QGraphicsView::mouseReleaseEvent(e);}
void CircuitCanvas::keyPressEvent(QKeyEvent* e){if(e->key()==Qt::Key_Shift&&preview_&&wireStart_){preview_->setPath(previewPath(wirePoint(lastPointer_,Qt::ShiftModifier)));e->accept();return;}if(e->key()==Qt::Key_Escape){cancelWire();e->accept();}else if(e->key()==Qt::Key_Delete){removeSelected();e->accept();}else QGraphicsView::keyPressEvent(e);}
void CircuitCanvas::wheelEvent(QWheelEvent* e){if(e->modifiers()&Qt::ControlModifier){zoom(e->angleDelta().y()>0?1.15:1/1.15);e->accept();}else QGraphicsView::wheelEvent(e);}
QPointF CircuitCanvas::snapped(QPointF pos)const{if(!snap_||(QApplication::keyboardModifiers()&Qt::AltModifier))return pos;return {std::round(pos.x()/gridSize_)*gridSize_,std::round(pos.y()/gridSize_)*gridSize_};}
void CircuitCanvas::drawBackground(QPainter* p,const QRectF& r){p->fillRect(r,palette().color(QPalette::Window));p->setPen(QPen(palette().color(QPalette::Mid),1));double visibleGrid=gridSize_;while(visibleGrid*transform().m11()<6)visibleGrid*=2;for(double x=std::floor(r.left()/visibleGrid)*visibleGrid;x<r.right();x+=visibleGrid)for(double y=std::floor(r.top()/visibleGrid)*visibleGrid;y<r.bottom();y+=visibleGrid)p->drawPoint(QPointF(x,y));}

QPointF CircuitCanvas::wirePoint(QPointF point,Qt::KeyboardModifiers modifiers)const{
    if(!wireStart_)return snapped(point);
    if(!(modifiers&Qt::ShiftModifier))return snapped(point);
    const auto origin=pendingBends_.empty()?pinPosition(*wireStart_):QPointF(pendingBends_.back().x,pendingBends_.back().y);
    const auto delta=point-origin;const double length=std::hypot(delta.x(),delta.y());
    const double angle=std::round(std::atan2(delta.y(),delta.x())/(3.141592653589793/4))*(3.141592653589793/4);
    return origin+QPointF(std::cos(angle),std::sin(angle))*length;
}
void CircuitCanvas::keyReleaseEvent(QKeyEvent* e){if(e->key()==Qt::Key_Shift&&preview_&&wireStart_)preview_->setPath(previewPath(wirePoint(lastPointer_,Qt::NoModifier)));QGraphicsView::keyReleaseEvent(e);}
QPainterPath CircuitCanvas::previewPath(QPointF end)const{QPainterPath path(pinPosition(*wireStart_));for(const auto& point:pendingBends_)path.lineTo(point.x,point.y);path.lineTo(end);return path;}
void CircuitCanvas::refreshBendHandles(){for(auto* item:scene()->items())if(item->data(0)=="bend")delete item;if(!editingEnabled_)return;for(auto* item:scene()->selectedItems())if(item->data(0)=="wire"){const auto id=item->data(1).toString().toStdString();for(const auto& wire:document_.wires())if(wire.id==id)for(std::size_t i=0;i<wire.points.size();++i)scene()->addItem(new BendHandle(this,id,i,{wire.points[i].x,wire.points[i].y}));}}
QPointF CircuitCanvas::constrainBend(const std::string& id,std::size_t index,QPointF point,Qt::KeyboardModifiers modifiers)const{if(!(modifiers&Qt::ShiftModifier))return snapped(point);const auto it=std::find_if(document_.wires().begin(),document_.wires().end(),[&](const auto& wire){return wire.id==id;});if(it==document_.wires().end()||index>=it->points.size())return point;const auto origin=index?QPointF(it->points[index-1].x,it->points[index-1].y):pinPosition(it->a);const auto delta=point-origin;const double length=std::hypot(delta.x(),delta.y()),angle=std::round(std::atan2(delta.y(),delta.x())/(3.141592653589793/4))*(3.141592653589793/4);return origin+QPointF(std::cos(angle),std::sin(angle))*length;}
void CircuitCanvas::moveBend(const std::string& id,std::size_t index,QPointF point,Qt::KeyboardModifiers modifiers){mutate(Language::text("Kabelknick verschieben"),[&](auto& doc){for(auto& wire:const_cast<std::vector<CircuitWire>&>(doc.wires()))if(wire.id==id){auto target=constrainBend(id,index,point,modifiers);wire.points.at(index)={target.x(),target.y()};}});}
void CircuitCanvas::addBend(const std::string& id,QPointF point){mutate(Language::text("Kabelknick hinzufügen"),[&](auto& doc){for(auto& wire:const_cast<std::vector<CircuitWire>&>(doc.wires()))if(wire.id==id){if(wire.points.size()>=64)throw std::runtime_error(Language::text("Maximal 64 Kabelknicke").toStdString());auto position=wire.points.end();double distance=1e100;QPointF previous=pinPosition(wire.a);for(std::size_t i=0;i<=wire.points.size();++i){const QPointF next=i<wire.points.size()?QPointF(wire.points[i].x,wire.points[i].y):pinPosition(wire.b);const double candidate=QLineF(previous,point).length()+QLineF(point,next).length()-QLineF(previous,next).length();if(candidate<distance){distance=candidate;position=wire.points.begin()+i;}previous=next;}auto target=snapped(point);wire.points.insert(position,{target.x(),target.y()});}});}
void CircuitCanvas::mouseDoubleClickEvent(QMouseEvent* event){if(!editingEnabled_){mousePressEvent(event);return;}auto* item=itemAt(event->pos());if(item&&item->data(0)=="wire"){addBend(item->data(1).toString().toStdString(),mapToScene(event->pos()));event->accept();return;}QGraphicsView::mouseDoubleClickEvent(event);}

void CircuitCanvas::changeValue(const std::string& id,double value){if(editingEnabled_){mutate(Language::text("Bauteilwert ändern"),[&](auto& doc){doc.part(id).value=value;});return;}auto next=document_;next.part(id).value=value;try{next.validate();document_=std::move(next);emit inputValueChanged();emit runtimeInputChanged();viewport()->update();}catch(const std::exception& error){emit notice(QString::fromUtf8(error.what()));}}

void CircuitCanvas::copySelected(){
    SavedProject fragment;fragment.circuit.removePart("board");fragment.sketch.order={"sketch.ino"};fragment.sketch.files["sketch.ino"]="void setup(){} void loop(){}";std::set<std::string> ids;
    for(auto* item:scene()->selectedItems())if(item->data(0)=="part"){auto id=item->data(1).toString().toStdString();ids.insert(id);fragment.circuit.insertPart(document_.part(id));}
    if(ids.empty())return;for(const auto& w:document_.wires())if(ids.contains(w.a.part)&&ids.contains(w.b.part)){fragment.circuit.connect(w.a,w.b,w.color);const_cast<CircuitWire&>(fragment.circuit.wires().back()).points=w.points;const_cast<CircuitWire&>(fragment.circuit.wires().back()).direct=w.direct;}
    if(clipboardSketches){auto sketches=clipboardSketches();for(auto it=sketches.begin();it!=sketches.end();++it)if(ids.contains(it.key().toStdString()))fragment.boardSketches[it.key()]=it.value();}
    auto* data=new QMimeData;data->setData("application/x-thinkcad-circuit",ProjectIO::serialize(fragment));QApplication::clipboard()->setMimeData(data);
}
void CircuitCanvas::paste(){
    if(!editingEnabled_)return;const auto* mime=QApplication::clipboard()->mimeData();if(!mime->hasFormat("application/x-thinkcad-circuit"))return;
    try{auto fragment=ProjectIO::deserialize(mime->data("application/x-thinkcad-circuit"));if(fragment.circuit.parts().empty())return;std::vector<std::string> created;QMap<QString,SketchProject> imported;mutate(Language::text("Bauteile einfügen"),[&](auto& doc){std::map<std::string,std::string> names;double left=1e9,top=1e9;for(const auto& p:fragment.circuit.parts()){left=std::min(left,p.x);top=std::min(top,p.y);}const auto center=snapped(mapToScene(viewport()->rect().center()));double dx=center.x()-left+30,dy=center.y()-top+30;
        for(const auto& p:fragment.circuit.parts()){auto id=doc.addPart(p.kind,p.x+dx,p.y+dy);auto copy=p;copy.id=id;copy.x+=dx;copy.y+=dy;doc.part(id)=copy;names[p.id]=id;created.push_back(id);if(fragment.boardSketches.contains(QString::fromStdString(p.id)))imported[QString::fromStdString(id)]=fragment.boardSketches.value(QString::fromStdString(p.id));}for(const auto& w:fragment.circuit.wires()){doc.connect({names.at(w.a.part),w.a.pin},{names.at(w.b.part),w.b.pin},w.color);auto& wire=const_cast<CircuitWire&>(doc.wires().back());wire.direct=w.direct;for(const auto& point:w.points)wire.points.push_back({point.x+dx,point.y+dy});}});if(importClipboardSketches&&!imported.isEmpty()&&std::all_of(created.begin(),created.end(),[&](const auto& id){return parts_.contains(id);}))importClipboardSketches(imported);scene()->clearSelection();for(const auto& id:created)if(parts_.contains(id))parts_.at(id)->setSelected(true);
    }catch(const std::exception& e){emit notice(QString::fromUtf8(e.what()));}
}
void CircuitCanvas::duplicateSelected(){copySelected();paste();}
void CircuitCanvas::alignSelected(bool horizontal){std::vector<std::string> ids;for(auto* item:scene()->selectedItems())if(item->data(0)=="part")ids.push_back(item->data(1).toString().toStdString());if(ids.size()<2)return;mutate(Language::text("Bauteile ausrichten"),[&](auto& doc){double sum=0;for(const auto& id:ids)sum+=horizontal?doc.part(id).y:doc.part(id).x;const double target=sum/ids.size();for(const auto& id:ids){if(horizontal)doc.part(id).y=target;else doc.part(id).x=target;}});}
void CircuitCanvas::distributeSelected(bool horizontal){std::vector<std::string> ids;for(auto* item:scene()->selectedItems())if(item->data(0)=="part")ids.push_back(item->data(1).toString().toStdString());if(ids.size()<3)return;auto coordinate=[&](const std::string& id){return horizontal?document_.part(id).x:document_.part(id).y;};std::sort(ids.begin(),ids.end(),[&](const auto& a,const auto& b){return coordinate(a)<coordinate(b);});const double start=coordinate(ids.front()),step=(coordinate(ids.back())-start)/(ids.size()-1);mutate(Language::text("Bauteile verteilen"),[&](auto& doc){for(std::size_t i=0;i<ids.size();++i){if(horizontal)doc.part(ids[i]).x=start+i*step;else doc.part(ids[i]).y=start+i*step;}});}
const NetTopology& CircuitCanvas::topology()const{const auto signature=NetTopology::signature(document_);if(!topology_||signature!=topologySignature_){topology_=std::make_unique<NetTopology>(document_);topologySignature_=signature;}return *topology_;}
void CircuitCanvas::clearNetPreview(){for(auto* item:netOverlays_){scene()->removeItem(item);delete item;}netOverlays_.clear();for(auto* item:scene()->items())if(dynamic_cast<PinItem*>(item)){item->setData(3,false);item->update();}}
void CircuitCanvas::previewNet(const Endpoint& ep){
    clearNetPreview();const auto& net=topology();if(!net.contains(ep))return;
    const auto members=wireStart_?net.preview(*wireStart_,ep):net.members(ep);std::set<Endpoint> joined(members.begin(),members.end());bool conflict=false;if(wireStart_){for(const auto& part:document_.parts()){if(part.kind==PartKind::Esp8266)conflict|=joined.contains({part.id,"GND"})&&(joined.contains({part.id,"3V3"})||joined.contains({part.id,"5V"}));if(part.kind==PartKind::BatteryAA||part.kind==PartKind::Battery3V||part.kind==PartKind::Battery9V||part.kind==PartKind::PowerSupply)conflict|=joined.contains({part.id,"+"})&&joined.contains({part.id,"-"});}if(conflict)emit notice(Language::text("Vorschau: möglicher Versorgungskurzschluss."));else if(net.connected(*wireStart_,ep))emit notice(Language::text("Diese Anschlüsse sind bereits elektrisch verbunden."));}
    for(auto* item:scene()->items())if(auto* pin=dynamic_cast<PinItem*>(item)){pin->setData(3,joined.contains(pin->endpoint()));pin->update();}
    for(const auto& wire:document_.wires())if(!wire.direct&&joined.contains(wire.a)){auto* item=scene()->addPath(routedWire(wire),QPen(QColor(conflict?"#dc3545":"#14b8a6"),4,Qt::DashLine));item->setAcceptedMouseButtons(Qt::NoButton);item->setZValue(20);item->setData(0,"netPreview");netOverlays_.push_back(item);}
}
void CircuitCanvas::highlightNet(const Endpoint& ep){previewNet(ep);}
void CircuitCanvas::selectTarget(const Endpoint& ep,const std::string& wire){scene()->clearSelection();if(parts_.contains(ep.part)){auto* item=parts_.at(ep.part);item->setSelected(true);ensureVisible(item,60,60);}if(!ep.pin.empty()&&topology().contains(ep))previewNet(ep);if(!wire.empty())for(auto* item:scene()->items())if(item->data(0)=="wire"&&item->data(1).toString().toStdString()==wire){item->setSelected(true);ensureVisible(item);}}

void CircuitCanvas::branchWire(const std::string& id,QPointF pos){
    std::string junction;mutate(Language::text("Abzweigung erstellen"),[&](auto& doc){auto it=std::find_if(doc.wires().begin(),doc.wires().end(),[&](const auto& w){return w.id==id;});if(it==doc.wires().end())throw std::runtime_error("Unknown wire");auto old=*it;std::vector<QPointF> path{pinPosition(old.a)};for(const auto& b:old.points)path.push_back({b.x,b.y});path.push_back(pinPosition(old.b));double best=1e100;std::size_t index=0;QPointF projection;
        for(std::size_t i=0;i+1<path.size();++i){auto delta=path[i+1]-path[i];double length=QPointF::dotProduct(delta,delta);double t=length?std::clamp(QPointF::dotProduct(pos-path[i],delta)/length,0.0,1.0):0;auto point=path[i]+delta*t;double distance=QLineF(point,pos).length();if(distance<best){best=distance;index=i;projection=point;}}
        junction=doc.addPart(PartKind::Junction,projection.x(),projection.y());doc.removeWire(id);doc.connect(old.a,{junction,"NET"},old.color);auto& first=const_cast<CircuitWire&>(doc.wires().back());first.points.assign(old.points.begin(),old.points.begin()+index);doc.connect({junction,"NET"},old.b,old.color);auto& second=const_cast<CircuitWire&>(doc.wires().back());second.points.assign(old.points.begin()+index,old.points.end());});if(!junction.empty()){selectPart(junction);connectPin({junction,"NET"});}
}

void CircuitCanvas::dragEnterEvent(QDragEnterEvent* e){if(editingEnabled_&&e->mimeData()->hasFormat("application/x-thinkcad-component"))e->acceptProposedAction();else e->ignore();}
void CircuitCanvas::clearPlacementPreview(){delete placementPreview_;placementPreview_=nullptr;}
void CircuitCanvas::dragLeaveEvent(QDragLeaveEvent* e){clearPlacementPreview();e->accept();}
void CircuitCanvas::dragMoveEvent(QDragMoveEvent* e){
    if(!editingEnabled_||!e->mimeData()->hasFormat("application/x-thinkcad-component")){e->ignore();return;}
    try{const auto json=QJsonDocument::fromJson(e->mimeData()->data("application/x-thinkcad-component")).object();const auto kind=static_cast<PartKind>(json["kind"].toInt(-1));(void)componentSpec(kind);auto trial=document_;auto point=snapped(mapToScene(e->position().toPoint()));const auto id=trial.addPart(kind,point.x(),point.y());trial.part(id).model=json["model"].toString().toStdString();seatContacts(trial,document_);clearPlacementPreview();auto* preview=new PlacementPreview(trial.part(id));for(const auto& w:trial.wires())if(w.direct){if(w.a.part==id)preview->seated.insert(w.a.pin);if(w.b.part==id)preview->seated.insert(w.b.pin);}placementPreview_=preview;scene()->addItem(preview);e->acceptProposedAction();}catch(const std::exception&){clearPlacementPreview();e->ignore();}
}
void CircuitCanvas::dropEvent(QDropEvent* e){
    clearPlacementPreview();
    if(!editingEnabled_||!e->mimeData()->hasFormat("application/x-thinkcad-component")){e->ignore();return;}
    try{auto json=QJsonDocument::fromJson(e->mimeData()->data("application/x-thinkcad-component")).object();if(!json["kind"].isDouble())throw std::invalid_argument("Invalid component drop");auto kind=static_cast<PartKind>(json["kind"].toInt(-1));(void)componentSpec(kind);auto model=json["model"].toString().toStdString();auto position=snapped(mapToScene(e->position().toPoint()));auto trial=document_;auto id=trial.addPart(kind,position.x(),position.y());trial.part(id).model=model;trial.validate();mutate(Language::text("Bauteil hinzufügen"),[&](auto& doc){doc=trial;});e->acceptProposedAction();}catch(const std::exception& error){emit notice(QString::fromUtf8(error.what()));e->ignore();}
}

void CircuitCanvas::editNote(const std::string& id){if(!editingEnabled_)return;QTimer::singleShot(0,this,[this,id]{if(!editingEnabled_||std::none_of(document_.parts().begin(),document_.parts().end(),[&](const auto& part){return part.id==id&&part.kind==PartKind::Note;}))return;bool ok=false;auto text=QInputDialog::getMultiLineText(this,Language::text("Notiz bearbeiten"),Language::text("Text"),QString::fromStdString(document_.part(id).model),&ok);if(ok)mutate(Language::text("Notiz bearbeiten"),[&](auto& doc){doc.part(id).model=text.toStdString();});});}

void CircuitCanvas::changeParameter(const std::string& id,const std::string& name,double value){
    const auto& part=document_.part(id);if(part.kind!=PartKind::Module)throw std::invalid_argument("Not a module parameter");const auto& model=ModuleRegistry::get(part.model);bool valid=false;
    for(const auto& field:model.parameters)if(field.name==name&&std::isfinite(value)&&value>=field.min&&value<=field.max&&(field.unit!="count"||value==std::floor(value)))valid=true;
    if(name=="address")valid=editingEnabled_&&model.bus=="i2c"&&value==std::floor(value)&&value>=(model.builtin?model.address:8)&&value<=(model.builtin?model.address+model.addressCount-1:119);
    if(name=="pixels"||name=="rows"||name=="columns")valid=valid&&editingEnabled_&&value==std::floor(value);
    if(!valid)throw std::invalid_argument("Module parameter outside its allowed range");if(parameter(part,name,name=="address"?model.address:0)==value&&part.parameters.contains(name))return;
    if(editingEnabled_)mutate(Language::text("Modulparameter ändern"),[&](auto& doc){doc.part(id).parameters[name]=value;if(name=="rows"||name=="columns"){auto pins=CircuitDocument::pins(doc.part(id));std::vector<std::string> remove;for(const auto& wire:doc.wires())for(const auto& ep:{wire.a,wire.b})if(ep.part==id&&std::find(pins.begin(),pins.end(),ep.pin)==pins.end())remove.push_back(wire.id);for(const auto& wire:remove)doc.removeWire(wire);}});else{document_.part(id).parameters[name]=value;viewport()->update();emit inputValueChanged();emit runtimeInputChanged();}
}
void CircuitCanvas::setKeyPressed(const std::string& id,int key,bool pressed){auto it=std::find_if(document_.parts().begin(),document_.parts().end(),[&](const auto& p){return p.id==id;});if(it==document_.parts().end()||(it->kind!=PartKind::Keypad&&it->kind!=PartKind::Keyboard)||key<0||key>=(it->kind==PartKind::Keypad?16:25))return;auto& p=document_.part(id);auto mask=static_cast<uint32_t>(parameter(p,"keys",0));if(pressed)mask|=1u<<key;else mask&=~(1u<<key);if(mask)p.parameters["keys"]=mask;else p.parameters.erase("keys");if(parts_.contains(id))parts_.at(id)->update();emit runtimeInputChanged();}

void CircuitCanvas::setEncoderInput(const std::string& id,int phase,int switchState){if(std::none_of(document_.parts().begin(),document_.parts().end(),[&](const auto& p){return p.id==id&&p.kind==PartKind::RotaryEncoder;}))return;auto& part=document_.part(id);if(phase>=0)part.parameters["phase"]=phase%4;if(switchState>=0)part.parameters["switch"]=switchState?1:0;emit runtimeInputChanged();}
