#include "componentdesign.hpp"
#include "language.hpp"
#include <QFont>
#include <QApplication>
#include <QPalette>
#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <QSettings>
#include <QStringList>
#include <QTransform>
#include <algorithm>
#include <array>
#include <cmath>
using namespace thinkercad;
namespace {
QString& preference(){static QString value=QSettings().value("componentDesign","blocks").toString()=="realistic"?"realistic":"blocks";return value;}
QRectF artBounds(const CircuitPart& part);
QPointF artPin(const CircuitPart& part,const std::string& pin);
QColor metal("#b9c4cd");
void label(QPainter& p,QRectF rect,const QString& text,QColor color=QColor("#334155"),int size=8){
    p.setPen(color);p.setFont(QFont("Sans Serif",size));p.drawText(rect,Qt::AlignCenter,text);
}
void body(QPainter& p,QRectF rect,QColor color,double radius=5){
    p.setPen(QPen(color.darker(160),1.2));QLinearGradient gradient(rect.topLeft(),rect.bottomRight());
    gradient.setColorAt(0,color.lighter(108));gradient.setColorAt(0.45,color);gradient.setColorAt(1,color.darker(108));
    p.setBrush(gradient);p.drawRoundedRect(rect,radius,radius);
}
void lead(QPainter& p,QPointF a,QPointF b){
    p.setPen(QPen(QColor("#87939e"),4,Qt::SolidLine,Qt::RoundCap));p.drawLine(a,b);
    p.setPen(QPen(QColor("#e5e9eb"),1.2));p.drawLine(a+QPointF(0,-1),b+QPointF(0,-1));
}
void screw(QPainter& p,QPointF center,double radius=6){
    p.setPen(QPen(QColor("#64717d"),1));p.setBrush(metal);p.drawEllipse(center,radius,radius);
    p.setPen(QPen(QColor("#52616e"),1.8));p.drawLine(center-QPointF(radius*0.65,0),center+QPointF(radius*0.65,0));
}
void chip(QPainter& p,QRectF rect,int contacts,const QString& name){
    const bool qfp=std::abs(rect.width()-rect.height())<8;
    for(int i=0;i<contacts;++i){double x=rect.left()+5+(rect.width()-10)*i/std::max(1,contacts-1);
        if(qfp){p.setPen(QPen(metal,1.5));p.drawLine(QPointF(x,rect.top()-3),QPointF(x,rect.bottom()+3));}
        else lead(p,{x,rect.top()-5},{x,rect.bottom()+5});
    }
    if(qfp)for(int i=0;i<contacts;++i){double y=rect.top()+5+(rect.height()-10)*i/std::max(1,contacts-1);p.setPen(QPen(metal,1.5));p.drawLine(QPointF(rect.left()-3,y),QPointF(rect.right()+3,y));}
    body(p,rect,QColor("#252a30"),2);p.setBrush(QColor("#47515b"));p.drawEllipse(rect.topLeft()+QPointF(7,7),2,2);
    label(p,rect,name,QColor("#d5d9dd"),7);
}
void usb(QPainter& p,QRectF rect){
    body(p,rect,QColor("#c7d0d8"),2);p.setBrush(QColor("#333b44"));p.setPen(Qt::NoPen);
    p.drawRoundedRect(rect.adjusted(4,2,-4,-rect.height()*0.45),1,1);
    p.setPen(QPen(QColor("#f0d091"),1.3));for(int i=0;i<5;++i){double x=rect.left()+rect.width()*0.25+i*rect.width()*0.125;p.drawLine(QPointF(x,rect.top()+4),QPointF(x,rect.top()+8));}
}
void dial(QPainter& p,QPointF center,double radius,double angle=0){
    p.setPen(QPen(QColor("#1f262b"),1));QLinearGradient finish(center-QPointF(radius,radius),center+QPointF(radius,radius));finish.setColorAt(0,QColor("#68757c"));finish.setColorAt(0.5,QColor("#343e44"));finish.setColorAt(1,QColor("#171f25"));p.setBrush(finish);p.drawEllipse(center,radius,radius);
    for(int i=0;i<16;++i){const double a=i*3.141592653589793/8;p.setPen(QPen(QColor("#78838a"),0.8));p.drawLine(center+QPointF(std::cos(a),std::sin(a))*(radius-3),center+QPointF(std::cos(a),std::sin(a))*(radius-1));}
    p.save();p.translate(center);p.rotate(angle);p.setPen(QPen(QColor("#eff3f5"),1.8,Qt::SolidLine,Qt::RoundCap));p.drawLine(QPointF(0,-radius+3),QPointF(0,-radius*0.45));p.restore();
}
void terminal(QPainter& p,QPointF point,QColor color){
    p.setPen(QPen(QColor("#454c52"),1));p.setBrush(QColor("#c4cbd0"));p.drawEllipse(point,7,7);p.setBrush(color);p.drawEllipse(point,5.5,5.5);p.setBrush(QColor("#11191e"));p.drawEllipse(point,2.3,2.3);
}
void header(QPainter& p,QRectF rect){body(p,rect,QColor("#202830"),2);}
void mountingHole(QPainter& p,QPointF point){p.setBrush(QColor("#c7ae73"));p.setPen(Qt::NoPen);p.drawEllipse(point,5,5);p.setBrush(QColor("#d9e0e5"));p.drawEllipse(point,2.7,2.7);}
QPointF boardPin(const CircuitPart& part,const std::string& pin){
    const auto profile=BoardProfile::get(part.model);
    if(profile.id=="arduino-uno"){
        if(pin=="5V")return {116,208};if(pin=="3V3")return {102,208};if(pin=="GND")return {130,208};if(pin=="GND2")return {144,208};
        const int number=profile.pinNumber(pin);
        if(number<14)return {284.0-number*14-(number>=8?8:0),12};
        return {200.0+(number-14)*14,208};
    }
    if(profile.id=="arduino-nano"){
        if(pin=="3V3")return {8,40};if(pin=="5V")return {8,200};if(pin=="GND")return {8,232};if(pin=="GND2")return {92,200};
        const int number=profile.pinNumber(pin);
        if(number==13)return {8,24};if(number>=14)return {8,72.0+(number-14)*16};
        return {92,24.0+(number<2?13+number:12-number)*16};
    }
    if(profile.id=="esp8266"){
        if(pin=="A0")return {8,24};if(pin=="GND")return {8,186};if(pin=="3V3")return {8,204};
        if(pin=="3V3_2")return {142,114};if(pin=="GND2")return {142,132};
        const auto number=profile.pinNumber(pin);
        const std::array<int,11> gpio{16,5,4,0,2,14,12,13,15,3,1};
        const auto found=std::find(gpio.begin(),gpio.end(),number);const int i=static_cast<int>(found-gpio.begin());
        return {142,24.0+(i<5?i:i+2)*18};
    }
    const auto names=CircuitDocument::pins(part);const int i=static_cast<int>(std::find(names.begin(),names.end(),pin)-names.begin());
    const int half=static_cast<int>((names.size()+1)/2);const auto rect=artBounds(part);
    const double spacing=(rect.height()-74)/std::max(1,half-1);
    return {i<half?8.0:142.0,24+(i%half)*spacing};
}
void board(QPainter& p,const CircuitPart& part,const CircuitState& state){
    const auto profile=BoardProfile::get(part.model);const auto extent=artBounds(part);
    const bool uno=profile.id=="arduino-uno",nano=profile.id=="arduino-nano";
    const QRectF pcb=uno?QRectF(0,0,300,220):nano?QRectF(0,0,100,272):QRectF(0,0,150,extent.height()-24);
    body(p,pcb,profile.arduino()?QColor("#087b9d"):QColor("#173c3b"),3);
    if(uno){
        header(p,{82,4,210,16});header(p,{50,200,118,16});header(p,{191,200,101,16});
        usb(p,{-10,38,50,42});body(p,{-8,145,40,34},QColor("#252c34"),3);
        p.setBrush(QColor("#10151b"));p.drawEllipse(QPointF(0,162),8,11);
        chip(p,{133,99,146,48},14,"ATmega328P");chip(p,{52,55,35,34},4,"USB");
        body(p,{84,143,20,25},QColor("#bcc5cc"),8);screw(p,{94,154},5);
        body(p,{44,17,20,14},QColor("#d85148"),2);
        label(p,{109,43,156,34},"UNO R3",Qt::white,20);
        label(p,{143,74,127,16},"5 V  ·  ATmega328P",QColor("#d9f1f0"),8);
        for(auto point:{QPointF(20,15),QPointF(278,38),QPointF(20,200),QPointF(282,183)})mountingHole(p,point);
        for(int i=0;i<4;++i){body(p,{45.0+i*17,177,10,7},QColor("#bba68c"),1);}
    }else if(nano){
        header(p,{1,15,14,244});header(p,{85,15,14,244});usb(p,{32,-7,36,36});
        chip(p,{34,111,32,32},8,"328P");body(p,{32,173,36,11},QColor("#bfc7ce"),5);
        body(p,{39,49,22,13},QColor("#b7b9bc"),2);p.setBrush(QColor("#ec7773"));p.drawEllipse(QPointF(50,55),4,4);
        label(p,{33,73,35,20},"NANO",Qt::white,8);label(p,{35,218,30,20},"5 V",Qt::white,8);
        mountingHole(p,{30,257});mountingHole(p,{70,257});
    }else{
        header(p,{1,15,14,pcb.height()-28});header(p,{135,15,14,pcb.height()-28});
        body(p,{32,67,86,109},QColor("#c3c9cd"),2);label(p,{37,100,76,45},profile.id=="esp8266"?"ESP8266":profile.id=="esp32-s3"?"ESP32-S3":"ESP32",QColor("#333e43"),10);
        body(p,{32,23,86,40},QColor("#25483c"),1);p.setPen(QPen(QColor("#d5b56e"),2));
        QPainterPath antenna({40,54});for(int i=0;i<5;++i){antenna.lineTo(40+i*15,31);antenna.lineTo(48+i*15,31);antenna.lineTo(48+i*15,54);}p.drawPath(antenna);
        chip(p,{57,pcb.height()-111,36,27},5,"USB");usb(p,{54,pcb.height()-28,42,32});
        for(double x:{31.0,101.0}){body(p,{x,pcb.height()-49,18,13},QColor("#bbc2c4"),2);p.setBrush(QColor("#343c44"));p.drawEllipse(QPointF(x+9,pcb.height()-42),4,4);}
        label(p,{22,pcb.height()-77,106,15},profile.id=="esp8266"?"NodeMCU":"DevKit",QColor("#d7eeee"),8);
        mountingHole(p,{23,8});mountingHole(p,{127,8});
    }
    p.setPen(QPen(QColor(80,150,129,85),0.65));p.setBrush(Qt::NoBrush);
    for(int i=0;i<7;++i){const double y=pcb.height()-28-i*6;QPainterPath route({18,y});route.lineTo(31+i*2,y);route.lineTo(44+i*2,y-9);route.lineTo(pcb.width()/2,y-9);p.drawPath(route);}
    for(int i=0;i<5;++i){const double x=uno?95+i*8:22+i*8;const double y=uno?172:pcb.height()-62;body(p,{x,y,5,2.5},QColor("#c6b88b"),0.3);}
    const bool lit=state.level({part.id,profile.pinName(profile.builtinLed)})==(profile.arduino()?Level::High:Level::Low);
    const QPointF ledPoint=uno?QPointF(108,25):nano?QPointF(54,203):QPointF(112,pcb.height()-70);
    p.setPen(Qt::NoPen);p.setBrush(lit?QColor("#67e89b"):QColor("#344f43"));p.drawRoundedRect(QRectF(ledPoint-QPointF(4,2),QSizeF(8,4)),1,1);
    // Physical header sockets include unmodeled power/reset positions in muted colors.
    auto socket=[&](QPointF point){p.setPen(QPen(QColor("#738777"),1));p.setBrush(QColor("#182b2e"));p.drawRoundedRect(QRectF(point-QPointF(3,3),QSizeF(6,6)),1,1);};
    if(nano)for(int i=0;i<15;++i){socket({8,24.0+i*16});socket({92,24.0+i*16});}
    if(profile.id=="esp8266")for(int i=0;i<15;++i){socket({8,24.0+i*18});socket({142,24.0+i*18});}
    if(uno)for(int i=0;i<8;++i)socket({60.0+i*14,208});
    for(const auto& name:CircuitDocument::pins(part)){
        const auto point=boardPin(part,name);auto text=QString::fromStdString(name);text.replace("GPIO","");
        if(text=="3V3_2")text="3V3";if(text=="GND2")text="GND";
        if(profile.id=="esp8266"&&name.starts_with("GPIO")){
            const int gpio=profile.pinNumber(name);if(gpio==3)text="RX";else if(gpio==1)text="TX";
            else for(std::size_t i=0;i<Esp8266Board::nodeMcuGpio.size();++i)if(gpio==Esp8266Board::nodeMcuGpio[i])text="D"+QString::number(i);
        }
        if(uno)label(p,{point.x()-8,point.y()<100?23.0:184.0,17,16},text,Qt::white,6);
        else label(p,{point.x()<50?(nano?15.0:16.0):pcb.width()-(nano?31:34),point.y()-7,nano?16.0:18.0,14},text,Qt::white,5);
    }
}
}
QString ComponentDesign::current(){return preference();}
void ComponentDesign::set(const QString& design){preference()=design=="realistic"?"realistic":"blocks";QSettings().setValue("componentDesign",preference());}
bool ComponentDesign::realistic(){return current()=="realistic";}
namespace {
QRectF artBounds(const CircuitPart& part){
    if(part.kind==PartKind::Esp8266){const auto profile=BoardProfile::get(part.model);
        if(profile.id=="arduino-uno")return {-14,-12,328,250};if(profile.id=="arduino-nano")return {-12,-12,124,310};
        return {-12,-12,174,static_cast<double>(std::max(330,70+static_cast<int>((CircuitDocument::pins(part).size()+1)/2)*16))};
    }
    if(part.kind==PartKind::Breadboard30)return {-12,-12,594,284};
    if(part.kind==PartKind::Oled)return {-12,-12,316,CircuitDocument::displayHeight(part)==32?160.0:228.0};
    switch(part.kind){
    case PartKind::Led:case PartKind::RGBLed:return {30,-2,100,126};
    case PartKind::Resistor:case PartKind::Diode:case PartKind::ReedSwitch:case PartKind::TiltSwitch:return {-5,36,170,44};
    case PartKind::Button:return {24,17,112,88};
    case PartKind::Potentiometer:return {30,18,100,107};
    case PartKind::Photoresistor:case PartKind::Thermistor:return {44,11,72,114};
    case PartKind::SlideSwitch:return {22,23,116,102};
    case PartKind::Battery3V:return {-5,6,170,113};
    case PartKind::Battery9V:return {34,-2,92,135};
    case PartKind::Voltmeter:return {25,0,110,145};
    case PartKind::PowerSupply:case PartKind::AnalogSource:return {8,9,144,117};
    case PartKind::SevenSegment:return {15,5,130,128};
    default:return {-12,-12,184,174};
    }
}
QPointF artPin(const CircuitPart& part,const std::string& pin){
    if(part.kind==PartKind::Esp8266)return boardPin(part,pin);
    const auto names=CircuitDocument::pins(part);const int i=static_cast<int>(std::find(names.begin(),names.end(),pin)-names.begin());
    switch(part.kind){
    case PartKind::Breadboard30:{const int column=std::stoi(pin.substr(1))-1,row=pin[0]-'a';return {20.0+column*18,40.0+row*18+(row>=5?25:0)};}
    case PartKind::Oled:return {47.0+i*66,CircuitDocument::displayHeight(part)==32?132.0:200.0};
    case PartKind::Led:case PartKind::Photoresistor:case PartKind::Thermistor:return {i?100.0:60.0,118};
    case PartKind::RGBLed:return {pin=="R"?50.0:pin=="K"?70.0:pin=="G"?90.0:110.0,118};
    case PartKind::Potentiometer:case PartKind::SlideSwitch:return {50.0+i*30,118};
    case PartKind::Button:return {i?130.0:30.0,44};
    case PartKind::Battery9V:return {i?98.0:62.0,12};
    case PartKind::PowerSupply:case PartKind::AnalogSource:return {i?110.0:50.0,102};
    case PartKind::Voltmeter:return {i?105.0:55.0,123};
    case PartKind::SevenSegment:{const int half=static_cast<int>((names.size()+1)/2);return {i<half?22.0:138.0,28.0+(i%half)*20};}
    default:return {i?160.0:0.0,58};
    }
}
// Shared scene scale: six scene units per millimetre. Dimensions describe
// representative modules, not an interchangeable manufacturer footprint.
QTransform physicalTransform(const CircuitPart& part){
    QSizeF drawing(1,1),millimetres(1,1);
    switch(part.kind){
    case PartKind::Esp8266:{const auto id=BoardProfile::get(part.model).id;
        if(id=="arduino-uno"){drawing={300,220};millimetres={68.6,53.4};}
        else if(id=="arduino-nano"){drawing={100,272};millimetres={18,45};}
        else{drawing={150,artBounds(part).height()-24};millimetres=id=="esp8266"?QSizeF(31,58):id=="esp32-s3"?QSizeF(25.4,69):QSizeF(25.4,54.6);}break;}
    case PartKind::Oled:drawing={292,CircuitDocument::displayHeight(part)==32?136.0:204.0};millimetres=CircuitDocument::displayHeight(part)==32?QSizeF(38,12):QSizeF(27.3,27.8);break;
    case PartKind::Breadboard30:drawing={570,260};millimetres={83.8,35.6};break;
    case PartKind::Led:case PartKind::RGBLed:drawing={60,66};millimetres={5,8.6};break;
    case PartKind::Resistor:drawing={86,30};millimetres={6.3,2.3};break;
    case PartKind::Button:drawing={76,76};millimetres={6,6};break;
    case PartKind::Potentiometer:drawing={88,65};millimetres={9.5,9.5};break;
    case PartKind::Photoresistor:case PartKind::Thermistor:drawing={60,60};millimetres={5,5};break;
    case PartKind::SlideSwitch:drawing={104,53};millimetres={8.5,3.5};break;
    case PartKind::ReedSwitch:drawing={100,30};millimetres={14,2.5};break;
    case PartKind::TiltSwitch:drawing={100,30};millimetres={10,4.5};break;
    case PartKind::Diode:drawing={100,30};millimetres={5.2,2.7};break;
    case PartKind::Battery3V:drawing={88,88};millimetres={20,20};break;
    case PartKind::Battery9V:drawing={80,107};millimetres={26.5,48.5};break;
    case PartKind::PowerSupply:case PartKind::AnalogSource:drawing={132,105};millimetres={90,70};break;
    case PartKind::Voltmeter:drawing={98,133};millimetres={75,150};break;
    case PartKind::SevenSegment:drawing={84,116};millimetres={12.7,19};break;
    }
    return QTransform::fromScale(6*millimetres.width()/drawing.width(),6*millimetres.height()/drawing.height());
}
}
QRectF ComponentDesign::bounds(const CircuitPart& part){return physicalTransform(part).mapRect(artBounds(part)).adjusted(-7,-7,7,7);}
QPointF ComponentDesign::pinPosition(const CircuitPart& part,const std::string& pin){if(part.kind==PartKind::Oled){const auto pins=CircuitDocument::pins(part);const auto i=std::find(pins.begin(),pins.end(),pin)-pins.begin();const bool small=CircuitDocument::displayHeight(part)==32;const double width=small?38:27.3,height=small?12:27.8;return {6*((width-7.62)/2+i*2.54),6*(height-1.3)};}if(part.kind==PartKind::Breadboard30){const int column=std::stoi(pin.substr(1))-1,row=pin[0]-'a';return {6*(3.5+column*2.54),6*(5.0+row*2.54+(row>=5?2.54:0))};}return physicalTransform(part).map(artPin(part,pin));}
void ComponentDesign::paint(QPainter& p,const CircuitPart& part,const CircuitState& state,const Frame& frame,double brightness,bool selected){
    p.save();p.setTransform(physicalTransform(part),true);p.setRenderHint(QPainter::Antialiasing);
    if(selected){p.setPen(QPen(QColor("#14b8a6"),1.5,Qt::DashLine));p.setBrush(Qt::NoBrush);p.drawRoundedRect(artBounds(part).adjusted(3,3,-3,-3),6,6);}
    const auto current=[&](const std::string& channel){auto it=state.ledCurrent.find(part.id+channel);return it==state.ledCurrent.end()?0.0:it->second;};
    switch(part.kind){
    case PartKind::Esp8266:board(p,part,state);break;
    case PartKind::Oled:{
        const bool small=CircuitDocument::displayHeight(part)==32;const double height=small?136:204;const auto scale=physicalTransform(part);const double screenHeight=244.0*CircuitDocument::displayHeight(part)/128.0*scale.m11()/scale.m22();
        body(p,{0,0,292,height},QColor("#145b89"),5);for(auto point:{QPointF(10,10),QPointF(282,10),QPointF(10,height-20),QPointF(282,height-20)})mountingHole(p,point);
        body(p,{18,19,256,screenHeight+12},QColor("#151d28"),2);p.setPen(Qt::NoPen);p.setBrush(QColor("#02080d"));p.drawRect(QRectF(24,24,244,screenHeight));
        const auto v=state.voltage.find({part.id,"VCC"}),g=state.voltage.find({part.id,"GND"});
        const bool powered=v!=state.voltage.end()&&g!=state.voltage.end()&&v->second&&g->second&&*v->second-*g->second>=3;
        if(powered){auto light=QColor("#87e8ff");light.setAlphaF(brightness);p.setBrush(light);const double sx=244.0/frame.width,sy=screenHeight/CircuitDocument::displayHeight(part);
            for(int y=0;y<std::min(frame.height,CircuitDocument::displayHeight(part));++y)for(int x=0;x<frame.width;++x)if(frame.pixel(x,y))p.drawRect(QRectF(24+x*sx,24+y*sy,sx*0.9,sy*0.9));}
        const double pitch=15.24/scale.m11();header(p,{146-2*pitch,height-15.24/scale.m22(),4*pitch,15.24/scale.m22()});
        if(!small)label(p,{30,height-52,232,16},"SSD1306",Qt::white,7);break;}
    case PartKind::Breadboard30:{
        body(p,{0,0,570,260},QColor("#eeeae1"),8);p.setPen(Qt::NoPen);p.setBrush(QColor("#c9c6bf"));p.drawRoundedRect(10,126,550,15,3,3);
        for(int i=0;i<30;++i){if(i==0||(i+1)%5==0)label(p,{12.0+i*18,9,18,19},QString::number(i+1),QColor("#77766f"),7);}
        for(int row=0;row<10;++row)label(p,{1,32.0+row*18+(row>=5?25:0),13,16},QString(QChar('a'+row)),QColor("#77766f"),6);
        for(int i=1;i<6;++i){p.setPen(QPen(QColor("#ddd8cd"),1));p.drawLine(i*95,3,i*95,256);}break;}
    case PartKind::Resistor:{
        lead(p,{0,58},{160,58});body(p,{37,43,86,30},QColor("#cfb58b"),9);
        const std::array<QColor,10> colors{QColor("#191d23"),QColor("#71432d"),QColor("#ca3335"),QColor("#e68a36"),QColor("#e8ce45"),QColor("#388653"),QColor("#386ba8"),QColor("#8055a2"),QColor("#8b9297"),QColor("#f3eee2")};
        const int exponent=static_cast<int>(std::floor(std::log10(std::max(1.0,part.value))))-1;
        const int digits=std::clamp(static_cast<int>(std::round(part.value/std::pow(10.0,exponent))),10,99);
        const std::array<QColor,4> bands{colors[digits/10],colors[digits%10],exponent<0?QColor("#bda258"):colors[std::clamp(exponent,0,9)],QColor("#bda258")};
        p.setPen(Qt::NoPen);for(int i=0;i<4;++i){p.setBrush(bands[i]);p.drawRect(48+i*19,44,6,28);}break;}
    case PartKind::Led:case PartKind::RGBLed:{
        for(const auto& name:CircuitDocument::pins(part)){const auto point=artPin(part,name);lead(p,{point.x(),76},point);}
        QColor color=QColor(QString::fromStdString(part.color));double light=current("");
        if(part.kind==PartKind::RGBLed){auto channel=[&](const char* name){return std::clamp(current(std::string(":")+name)/0.02,0.0,1.0);};color=QColor::fromRgbF(channel("R"),channel("G"),channel("B"));light=std::max({channel("R"),channel("G"),channel("B")});}
        if(light>1e-5){auto glow=color;glow.setAlpha(42);p.setPen(Qt::NoPen);p.setBrush(glow);p.drawEllipse(QPointF(80,49),48,48);}
        QPainterPath dome({50,77});dome.lineTo(50,44);dome.cubicTo(50,8,110,8,110,44);dome.lineTo(110,77);dome.closeSubpath();
        QLinearGradient gradient(50,25,110,73);gradient.setColorAt(0,color.lighter(180));gradient.setColorAt(0.5,light>1e-5?color:color.darker(150));gradient.setColorAt(1,color.darker(210));p.setBrush(gradient);p.setPen(QPen(color.darker(200),1));p.drawPath(dome);
        body(p,{45,73,70,9},color,3);p.setPen(QPen(QColor(255,255,255,150),4,Qt::SolidLine,Qt::RoundCap));p.drawLine(61,33,61,51);break;}
    case PartKind::Potentiometer:{
        for(const auto& name:CircuitDocument::pins(part)){auto point=artPin(part,name);lead(p,{point.x(),78},point);}
        body(p,{36,24,88,65},QColor("#245a91"),2);p.setPen(QPen(QColor("#6395b9"),1));p.drawLine(39,28,120,28);p.drawLine(39,28,39,84);
        body(p,{89,28,25,25},QColor("#ad9459"),1);p.save();p.translate(101.5,40.5);p.rotate(part.value*2.7-135);p.setPen(QPen(QColor("#554627"),2.4));p.drawLine(-8,0,8,0);p.restore();
        label(p,{44,58,70,17},"3296  103",QColor("#d5dfed"),7);break;}
    case PartKind::Photoresistor:case PartKind::Thermistor:{
        lead(p,{60,60},{60,118});lead(p,{100,60},{100,118});
        p.setPen(QPen(QColor("#665243"),1.5));p.setBrush(part.kind==PartKind::Photoresistor?QColor("#c9a87a"):QColor("#354c67"));p.drawEllipse(QPointF(80,47),30,30);
        if(part.kind==PartKind::Photoresistor){p.setPen(QPen(QColor("#7b4339"),4,Qt::SolidLine,Qt::RoundCap));QPainterPath trace({61,29});for(int i=0;i<5;++i){trace.lineTo(i%2?97:63,32+i*8);trace.lineTo(i%2?97:63,39+i*8);}p.drawPath(trace);}
        else label(p,{55,33,50,25},"10K",QColor("#e3ecef"),11);
        break;}
    case PartKind::Button:{
        for(int y:{44,82})lead(p,{30.0,double(y)},{130.0,double(y)});
        body(p,{42,23,76,76},QColor("#8d969d"),4);for(auto point:{QPointF(49,30),QPointF(111,30),QPointF(49,92),QPointF(111,92)})screw(p,point,3);
        p.setPen(QPen(QColor("#161e26"),2));p.setBrush(part.pressed?QColor("#172c36"):QColor("#343d46"));p.drawEllipse(QPointF(80,61),25,25);
        p.setPen(QPen(QColor("#6c7a83"),2));p.drawArc(QRectF(59,40,42,42),20*16,140*16);break;}
    case PartKind::SlideSwitch:{
        for(const auto& name:CircuitDocument::pins(part)){auto point=artPin(part,name);lead(p,{point.x(),72},point);}
        body(p,{28,29,104,53},QColor("#8c969d"),3);body(p,{39,38,82,29},QColor("#262e35"),2);body(p,{part.pressed?86.0:43.0,34,31,41},QColor("#343e48"),2);
        p.setPen(QPen(QColor("#74808c"),1));for(int i=0;i<4;++i){double x=(part.pressed?90:47)+i*6;p.drawLine(QPointF(x,40),QPointF(x,69));}break;}
    case PartKind::TiltSwitch:case PartKind::ReedSwitch:case PartKind::Diode:{
        lead(p,{0,58},{160,58});const bool reed=part.kind==PartKind::ReedSwitch,tilt=part.kind==PartKind::TiltSwitch;
        body(p,{35,41,90,34},tilt?QColor("#4b94b8"):reed?QColor("#b6d2bd"):QColor("#343b42"),tilt?8:12);
        if(reed){p.setPen(QPen(QColor("#53665b"),3));p.drawLine(38,58,78,58);p.drawLine(QPointF(82,part.pressed?58:50),QPointF(122,58));}
        else if(tilt){p.setPen(Qt::NoPen);p.setBrush(metal);p.drawEllipse(QPointF(part.pressed?107:51,58),10,10);}
        else{p.setPen(QPen(QColor("#cbd3d9"),6));p.drawLine(110,43,110,73);label(p,{46,44,53,28},"Si",Qt::white,9);}break;}
    case PartKind::Battery3V:{
        lead(p,{0,58},{160,58});body(p,{28,12,104,100},QColor("#30383c"),18);p.setPen(QPen(QColor("#7f8b96"),2));
        QLinearGradient gradient(45,20,115,100);gradient.setColorAt(0,QColor("#eef1f2"));gradient.setColorAt(0.5,QColor("#aeb8c1"));gradient.setColorAt(1,QColor("#e3e9ed"));p.setBrush(gradient);p.drawEllipse(QPointF(80,59),44,44);
        label(p,{47,29,66,27},"+",QColor("#56616b"),21);label(p,{44,63,72,25},"CR2032 3V",QColor("#56616b"),9);break;}
    case PartKind::Battery9V:{
        body(p,{40,20,80,107},QColor("#333b42"),6);body(p,{40,20,80,35},QColor("#bb985c"),5);screw(p,{62,12},8);
        p.setBrush(metal);p.setPen(QPen(QColor("#707e8b"),2));QPolygonF terminal;for(int i=0;i<6;++i){double angle=i*3.141592653589793/3;terminal<<QPointF(98+10*std::cos(angle),12+10*std::sin(angle));}p.drawPolygon(terminal);
        label(p,{51,58,58,46},"9 V",Qt::white,24);label(p,{42,29,74,18},"+          −",QColor("#302a21"),9);break;}
    case PartKind::PowerSupply:case PartKind::AnalogSource:{
        const bool supply=part.kind==PartKind::PowerSupply;body(p,{14,15,132,105},QColor("#b9c1c6"),2);body(p,{19,20,122,94},QColor("#d8dcdc"),1);
        for(const auto& point:{QPointF(21,22),QPointF(139,22),QPointF(21,112),QPointF(139,112)})screw(p,point,1.7);
        for(int i=0;i<9;++i){p.setPen(QPen(QColor("#8f999e"),1));p.drawLine(30+i*5,24,32+i*5,24);}
        body(p,{26,34,74,32},QColor("#242e32"),1);body(p,{30,38,66,24},supply?QColor("#a4b49c"):QColor("#1a2c36"),0.5);
        label(p,{32,40,62,20},QString("%1 V").arg(part.value,0,'f',2),supply?QColor("#233229"):QColor("#86d2b9"),10);
        dial(p,{120,48},9,35);dial(p,{120,77},7,-20);label(p,{101,58,38,10},supply?"VOLTAGE":"LEVEL",QColor("#3e494f"),4);
        label(p,{29,70,64,10},supply?"DC POWER SUPPLY":"SIGNAL SOURCE",QColor("#364047"),4);
        body(p,{29,87,13,9},QColor("#3b4847"),0.7);label(p,{27,78,17,8},"ON",QColor("#424a4f"),4);
        terminal(p,{50,102},QColor("#a23737"));terminal(p,{110,102},QColor("#252b2f"));label(p,{64,95,30,12},supply?"OUTPUT":"OUT",QColor("#364047"),4);break;}
    case PartKind::Voltmeter:{
        body(p,{31,6,98,133},QColor("#c79731"),6);body(p,{37,11,86,123},QColor("#2c3439"),4);label(p,{47,14,66,10},"DIGITAL MULTIMETER",QColor("#b7c1c3"),4);
        body(p,{43,27,74,31},QColor("#14201c"),1);body(p,{46,30,68,25},QColor("#b1be9d"),0.5);
        const auto a=state.voltage.find({part.id,"+"}),b=state.voltage.find({part.id,"-"});const QString text=a!=state.voltage.end()&&b!=state.voltage.end()&&a->second&&b->second?QString::number(*a->second-*b->second,'f',2):"—";
        label(p,{48,31,64,22},text,QColor("#2d3b2b"),12);label(p,{95,48,15,6},"V DC",QColor("#3b4637"),3);
        dial(p,{80,85},19,-25);
        for(int i=0;i<7;++i){const double angle=(-150+i*50)*3.141592653589793/180;const QPointF point=QPointF(80,85)+QPointF(std::cos(angle),std::sin(angle))*27;label(p,{point.x()-8,point.y()-4,16,8},QStringList{"V DC","200","20","2","Ω","V AC","OFF"}[i],QColor("#c3cacd"),3);}
        terminal(p,{55,123},QColor("#a23737"));terminal(p,{105,123},QColor("#252b2f"));label(p,{62,117,38,12},"V Ω  COM",QColor("#c3cacd"),3);break;}
    case PartKind::SevenSegment:{
        const auto names=CircuitDocument::pins(part);for(const auto& name:names){auto point=artPin(part,name);lead(p,point,{point.x()<80?45.0:115.0,point.y()});}
        body(p,{38,11,84,116},QColor("#292e35"),4);body(p,{44,17,72,104},QColor("#11171e"),2);
        const std::array<QLineF,7> segments{QLineF(59,30,96,30),QLineF(101,35,101,62),QLineF(101,72,101,100),QLineF(58,105,96,105),QLineF(53,72,53,100),QLineF(53,35,53,62),QLineF(59,67,96,67)};
        for(int i=0;i<7;++i){p.setPen(QPen(current(":"+std::string(1,char('a'+i)))>1e-5?QColor("#ff524c"):QColor("#41272c"),6,Qt::SolidLine,Qt::RoundCap));p.drawLine(segments[i]);}
        p.setPen(Qt::NoPen);p.setBrush(current(":dp")>1e-5?QColor("#ff524c"):QColor("#41272c"));p.drawEllipse(QPointF(109,110),3,3);break;}
    }
    p.restore();
}
