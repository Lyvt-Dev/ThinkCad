#include "breadboardlayout.hpp"
#include "physicalparts.hpp"
#include "componentdesign.hpp"
#include "thinkercad/modules.hpp"
#include "notelayout.hpp"
#include "language.hpp"
#include <QFont>
#include <QApplication>
#include <QPalette>
#include <QFontMetricsF>
#include <QPainter>
#include <QPainterPath>
#include <QSettings>
#include <QStringList>
#include <QTransform>
#include <QLineF>
#include <stdexcept>
#include <algorithm>
#include <array>
#include <cmath>
using namespace thinkercad;
namespace {
QString& preference(){static QString value=QSettings().value("componentDesign","blocks").toString()=="realistic"?"realistic":"blocks";return value;}
QRectF artBounds(const CircuitPart& part);
QPointF artPin(const CircuitPart& part,const std::string& pin);
QTransform physicalTransform(const CircuitPart& part);
QColor metal("#b9c4cd");
constexpr double unoPitch=15.24/(6*68.6/300),nanoPitch=15.24/(6*45./272);
void label(QPainter& p,QRectF rect,const QString& text,QColor color=QColor("#334155"),int size=8){
    QFont font("Sans Serif");font.setPixelSize(std::max(3,size));
    QFontMetricsF metrics(font);
    const double fit=std::min({1.0,(rect.width()-2)/std::max(1.0,metrics.horizontalAdvance(text)),(rect.height()-2)/metrics.height()});
    font.setPixelSize(std::max(2,static_cast<int>(size*fit)));
    p.save();p.setClipRect(rect);p.setPen(color);p.setFont(font);p.drawText(rect,Qt::AlignCenter,text);p.restore();
}
void body(QPainter& p,QRectF rect,QColor color,double radius=5){
    p.setPen(QPen(color.darker(135),0.8));p.setBrush(color);p.drawRoundedRect(rect,radius,radius);
}
void lead(QPainter& p,QPointF a,QPointF b){
    p.setPen(QPen(QColor("#87939e"),4,Qt::SolidLine,Qt::RoundCap));p.drawLine(a,b);
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
    p.setPen(QPen(QColor("#1f262b"),1));p.setBrush(QColor("#46535b"));p.drawEllipse(center,radius,radius);
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
        // Arduino UNO-TH Rev3e reference CAD, viewed from the component side.
        QPointF mm;
        if(pin=="5V")mm={38.1,50.8};else if(pin=="3V3")mm={35.56,50.8};
        else if(pin=="GND")mm={40.64,50.8};else if(pin=="GND2")mm={43.18,50.8};else if(pin=="GND3")mm={26.416,2.54};
        else if(pin=="SDA")mm={21.336,2.54};else if(pin=="SCL")mm={18.796,2.54};
        else{const int number=profile.pinNumber(pin);mm=number<14?QPointF(63.5-number*2.54-(number>=8?1.524:0),2.54):QPointF(50.8+(number-14)*2.54,50.8);}
        return physicalTransform(part).inverted().map(mm*6);
    }
    if(profile.id=="arduino-nano"){
        if(pin=="3V3")return {8,24+nanoPitch};if(pin=="5V")return {8,24+11*nanoPitch};if(pin=="GND")return {8,24+13*nanoPitch};if(pin=="GND2")return {92.6666667,24+11*nanoPitch};
        const int number=profile.pinNumber(pin);
        if(number==13)return {8,24};if(number>=14)return {8,24+(number-11)*nanoPitch};
        return {92.6666667,24+(number<2?13+number:12-number)*nanoPitch};
    }
    if(profile.id=="esp32"||profile.id=="esp32-s3"){
        const bool s3=profile.id=="esp32-s3";const double h=s3?62.74:48.26;const int rows=s3?22:19;const double top=s3?1.4:(h-(rows-1)*2.54)/2;
        const std::vector<int> left=s3?std::vector<int>{-1,-1,-1,4,5,6,7,15,16,17,18,8,3,46,9,10,11,12,13,14,-1,-1}:std::vector<int>{-1,-1,36,39,34,35,32,33,25,26,27,14,12,-1,13,-1,-1,-1,-1};
        const std::vector<int> right=s3?std::vector<int>{-1,43,44,1,2,42,41,40,39,38,37,36,35,0,45,48,47,21,20,19,-1,-1}:std::vector<int>{-1,23,22,1,3,21,-1,19,18,5,17,16,4,0,2,15,-1,-1,-1};
        QPointF mm;
        if(pin=="3V3"||pin=="3V3_2")mm={1.27,top+(pin=="3V3_2"&&s3?2.54:0)};
        else if(pin=="GND")mm={1.27,top+(s3?21:13)*2.54};
        else if(pin=="GND2")mm={s3?24.13:26.67,top};
        else{const int gpio=pin=="A0"?profile.defaultAdc:profile.pinNumber(pin);auto l=std::find(left.begin(),left.end(),gpio),r=std::find(right.begin(),right.end(),gpio);if(l!=left.end())mm={1.27,top+double(l-left.begin())*2.54};else if(r!=right.end())mm={s3?24.13:26.67,top+double(r-right.begin())*2.54};else throw std::invalid_argument("GPIO has no modeled header position");}
        return physicalTransform(part).inverted().map(mm*6);
    }
    const auto transform=physicalTransform(part);const double pitch=15.24/transform.m22();
    const double right=8+std::round(134*transform.m11()/15.24)*15.24/transform.m11();
    if(profile.id=="esp8266"){
        const double top=(artBounds(part).height()-24-14*pitch)/2;
        if(pin=="A0")return {8,top};if(pin=="GND")return {8,top+9*pitch};if(pin=="3V3")return {8,top+10*pitch};
        if(pin=="3V3_2")return {right,top+5*pitch};if(pin=="GND2")return {right,top+6*pitch};
        const auto number=profile.pinNumber(pin);const std::array<int,11> gpio{16,5,4,0,2,14,12,13,15,3,1};
        const auto found=std::find(gpio.begin(),gpio.end(),number);const int i=static_cast<int>(found-gpio.begin());return {right,top+(i<5?i:i+2)*pitch};
    }
    const auto names=CircuitDocument::pins(part);const int i=static_cast<int>(std::find(names.begin(),names.end(),pin)-names.begin());
    const int half=static_cast<int>((names.size()+1)/2);const double top=(artBounds(part).height()-24-(half-1)*pitch)/2;
    return {i<half?8.:right,top+(i%half)*pitch};
}
void board(QPainter& p,const CircuitPart& part,const CircuitState& state){
    const auto profile=BoardProfile::get(part.model);const auto extent=artBounds(part);
    const bool uno=profile.id=="arduino-uno",nano=profile.id=="arduino-nano";
    const QRectF pcb=uno?QRectF(0,0,300,220):nano?QRectF(0,0,100,272):QRectF(0,0,150,extent.height()-24);
    if(uno){
        QPainterPath outline({0,0});outline.lineTo(288,0);outline.lineTo(288,48);outline.lineTo(300,60);outline.lineTo(300,197);outline.lineTo(287,210);outline.lineTo(287,220);outline.lineTo(0,220);outline.closeSubpath();
        p.setBrush(QColor("#287c9d"));p.setPen(Qt::NoPen);p.drawPath(outline);
    }else body(p,pcb,profile.arduino()?QColor("#087b9d"):QColor("#424649"),3);
    if(uno){
        header(p,{82,4,210,16});header(p,{50,200,118,16});header(p,{191,200,101,16});
        usb(p,{-10,38,50,42});body(p,{-8,145,40,34},QColor("#252c34"),3);
        p.setBrush(QColor("#10151b"));p.drawEllipse(QPointF(0,162),8,11);
        chip(p,{133,99,146,48},14,"ATmega328P");chip(p,{52,55,35,34},4,"USB");
        body(p,{84,143,20,25},QColor("#bcc5cc"),8);screw(p,{94,154},5);
        body(p,{44,17,20,14},QColor("#d85148"),2);
        label(p,{109,65,156,27},"UNO R3",Qt::white,20);
        label(p,{143,151,127,16},"5 V  ·  ATmega328P",QColor("#d9f1f0"),8);
        for(auto point:{QPointF(20,15),QPointF(278,38),QPointF(20,200),QPointF(282,183)})mountingHole(p,point);
        for(int i=0;i<4;++i){body(p,{45.0+i*17,177,10,7},QColor("#bba68c"),1);}
    }else if(nano){
        header(p,{1,15,14,244});header(p,{85,15,14,244});usb(p,{32,-7,36,36});
        chip(p,{34,111,32,32},8,"328P");body(p,{32,173,36,11},QColor("#bfc7ce"),5);
        body(p,{39,49,22,13},QColor("#b7b9bc"),2);p.setBrush(QColor("#ec7773"));p.drawEllipse(QPointF(50,55),4,4);
        label(p,{33,73,35,20},"NANO",Qt::white,8);label(p,{35,218,30,20},"5 V",Qt::white,8);
        mountingHole(p,{30,257});mountingHole(p,{70,257});
    }else if(profile.id=="rp2040"||profile.id=="stm32f103"){
        body(p,pcb,profile.id=="rp2040"?QColor("#28745a"):QColor("#23649a"),3);header(p,{1,15,14,pcb.height()-28});header(p,{135,15,14,pcb.height()-28});usb(p,{54,-4,42,32});chip(p,{46,pcb.height()*.37,58,58},8,profile.id=="rp2040"?"RP2040":"F103");label(p,{20,pcb.height()*.68,110,24},profile.id=="rp2040"?"Raspberry Pi Pico":"STM32F103",Qt::white,10);body(p,{58,48,32,20},QColor("#d2d2cb"),2);label(p,{56,71,38,12},profile.id=="rp2040"?"BOOTSEL":"RESET",Qt::white,7);
    }else{
        header(p,{1,15,14,pcb.height()-28});header(p,{135,15,14,pcb.height()-28});
        body(p,{32,67,86,109},QColor("#c3c9cd"),2);label(p,{37,100,76,45},QString::fromStdString(profile.id).toUpper(),QColor("#333e43"),10);
        body(p,{32,23,86,40},QColor("#25483c"),1);p.setPen(QPen(QColor("#d5b56e"),2));
        QPainterPath antenna({40,54});for(int i=0;i<5;++i){antenna.lineTo(40+i*15,31);antenna.lineTo(48+i*15,31);antenna.lineTo(48+i*15,54);}p.setBrush(Qt::NoBrush);p.drawPath(antenna);
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
    if(nano)for(int i=0;i<15;++i){socket({8,24.+i*nanoPitch});socket({92.6666667,24.+i*nanoPitch});}
    if(profile.id=="esp8266"){const auto transform=physicalTransform(part);const double pitch=15.24/transform.m22(),top=(extent.height()-24-14*pitch)/2,right=8+std::round(134*transform.m11()/15.24)*15.24/transform.m11();for(int i=0;i<15;++i){socket({8,top+i*pitch});socket({right,top+i*pitch});}}
    if(uno)for(int i=0;i<8;++i)socket({60.+i*unoPitch,208});
    for(const auto& name:CircuitDocument::pins(part)){
        const auto point=boardPin(part,name);auto text=QString::fromStdString(name);text.replace("GPIO","");
        if(text=="3V3_2")text="3V3";if(text=="GND2"||text=="GND3")text="GND";
        if(profile.id=="esp8266"&&name.starts_with("GPIO")){
            const int gpio=profile.pinNumber(name);if(gpio==3)text="RX";else if(gpio==1)text="TX";
            else for(std::size_t i=0;i<Esp8266Board::nodeMcuGpio.size();++i)if(gpio==Esp8266Board::nodeMcuGpio[i])text="D"+QString::number(i);
        }
        if(profile.supportsPwm(profile.pinNumber(name)))text.prepend("~");
        if(uno){
            // Each label occupies its own header-pitch column, turned inward like silkscreen.
            p.save();p.translate(point.x(),point.y()<100?24:196);p.rotate(-90);
            label(p,point.y()<100?QRectF(-35,-5,34,10):QRectF(1,-5,34,10),text,Qt::white,7);p.restore();
        }else label(p,{point.x()<50?17.0:pcb.width()-(nano?42:43),point.y()-6,nano?25.0:26.0,12},text,Qt::white,7);
    }
}
}
QString ComponentDesign::current(){return preference();}
void ComponentDesign::set(const QString& design){preference()=design=="realistic"?"realistic":"blocks";QSettings().setValue("componentDesign",preference());}
bool ComponentDesign::realistic(){return current()=="realistic";}
namespace {
QRectF artBounds(const CircuitPart& part){if(poweredBoard(part))return {-6,-6,515,342};if(part.kind==PartKind::Note)return noteBounds(part);
    if(part.kind==PartKind::CharacterLcd||part.kind==PartKind::I2cCharacterLcd)return {-10,-10,250,160};
    if(part.kind==PartKind::Junction)return {-12,-12,24,24};if(part.kind==PartKind::NetLabel)return {-8,-26,150,42};
    if(part.kind==PartKind::Esp8266){const auto profile=BoardProfile::get(part.model);
        if(profile.id=="arduino-uno")return {-14,-12,328,250};if(profile.id=="arduino-nano")return {-12,-12,124,310};
        return {-12,-12,174,static_cast<double>(std::max(330,70+static_cast<int>((CircuitDocument::pins(part).size()+1)/2)*16))};
    }
    if(part.kind==PartKind::Breadboard30)return {-12,-12,594,284};
    if(part.kind==PartKind::Oled)return {-12,-12,316,CircuitDocument::displayHeight(part)==32?160.0:228.0};
    switch(part.kind){
    case PartKind::RotaryEncoder:return {0,0,160,170};
    case PartKind::Keypad:return {0,0,160,180};
    case PartKind::Keyboard:return {0,0,400,158};
    case PartKind::Capacitor:case PartKind::Inductor:return {28,8,104,136};
    case PartKind::NpnTransistor:case PartKind::PnpTransistor:case PartKind::NMosfet:case PartKind::PMosfet:return {30,5,100,140};
    case PartKind::OpAmp:return {0,0,160,130};
    case PartKind::Ground:return {20,20,120,100};
    case PartKind::FunctionGenerator:case PartKind::Oscilloscope:case PartKind::LogicAnalyzer:case PartKind::CurrentProbe:return {-4,0,168,150};
    case PartKind::InfraredLed:case PartKind::Led:case PartKind::RGBLed:return {30,-2,100,150};
    case PartKind::PiezoBuzzer:case PartKind::Speaker:return {22,5,116,140};
    case PartKind::TemperatureSensor:return {38,10,84,135};
    case PartKind::BatteryAA:return {30,2,100,150};
    case PartKind::ToggleSwitch:return {20,20,120,106};
    case PartKind::Resistor:case PartKind::Diode:case PartKind::ReedSwitch:case PartKind::TiltSwitch:return {-5,36,170,44};
    case PartKind::Button:return {24,17,112,88};
    case PartKind::Potentiometer:return {30,18,100,107};
    case PartKind::Photoresistor:case PartKind::Thermistor:return {44,11,72,114};
    case PartKind::SlideSwitch:return {22,23,116,102};
    case PartKind::Battery3V:return {-5,6,170,113};
    case PartKind::Battery9V:return {34,-2,92,135};
    case PartKind::Voltmeter:return {25,0,110,208};
    case PartKind::PowerSupply:case PartKind::AnalogSource:return {8,9,144,117};
    case PartKind::SevenSegment:return {15,5,130,128};
    default:return {-12,-12,184,174};
    }
}
QPointF artPin(const CircuitPart& part,const std::string& pin){
    if(part.kind==PartKind::CharacterLcd||part.kind==PartKind::I2cCharacterLcd){const auto names=CircuitDocument::pins(part);auto i=std::find(names.begin(),names.end(),pin)-names.begin();return {10.+i*(part.kind==PartKind::CharacterLcd?14.:60.),140};}
    if(part.kind==PartKind::Junction||part.kind==PartKind::NetLabel)return {0,0};
    if(part.kind==PartKind::Esp8266)return boardPin(part,pin);
    const auto names=CircuitDocument::pins(part);const int i=static_cast<int>(std::find(names.begin(),names.end(),pin)-names.begin());
    switch(part.kind){
    case PartKind::RotaryEncoder:return {50.+i*20,160};
    case PartKind::Keypad:return {59.68+i*5.8057,174};
    case PartKind::Keyboard:return {172.06+i*5.08,150};
    case PartKind::Breadboard30:{const int column=std::stoi(pin.substr(1))-1,row=pin[0]-'a';return {20.0+column*18,40.0+row*18+(row>=5?25:0)};}
    case PartKind::Oled:return {47.0+i*66,CircuitDocument::displayHeight(part)==32?132.0:200.0};
    case PartKind::Capacitor:case PartKind::Inductor:return {i?100.0:60.0,136};
    case PartKind::NpnTransistor:case PartKind::PnpTransistor:case PartKind::NMosfet:case PartKind::PMosfet:return {50.0+i*30,138};
    case PartKind::OpAmp:return i==0?QPointF(8,45):i==1?QPointF(8,85):i==2?QPointF(80,8):i==3?QPointF(80,122):QPointF(152,65);
    case PartKind::Ground:return {80,26};
    case PartKind::FunctionGenerator:case PartKind::CurrentProbe:return {i?115.0:45.0,138};
    case PartKind::Oscilloscope:return {40.0+i*40,138};
    case PartKind::LogicAnalyzer:return {16.0+i*16,138};
    case PartKind::InfraredLed:case PartKind::Led:return {i?95.24:64.76,118.0};
    case PartKind::InfraredReceiver:return {50.+i*30,145};
    case PartKind::RadioTransceiver:return {25.+(i%4)*36,130.+(i/4)*20};
    case PartKind::RadioTransmitter:case PartKind::RadioReceiver:return {40.+i*40,145};
    case PartKind::PiezoBuzzer:case PartKind::Speaker:return {i?105.0:55.0,138};
    case PartKind::TemperatureSensor:return {50.0+i*30,138};
    case PartKind::BatteryAA:return {80,i?145.0:8.0};
    case PartKind::ToggleSwitch:return {i?120.0:40.0,118};
    case PartKind::Photoresistor:case PartKind::Thermistor:return {i?100.0:60.0,118};
    case PartKind::RGBLed:return {pin=="R"?50.0:pin=="K"?70.0:pin=="G"?90.0:110.0,118};
    case PartKind::Potentiometer:case PartKind::SlideSwitch:return {50.0+i*30,118};
    case PartKind::Button:return {i?130.0:30.0,44};
    case PartKind::Battery9V:return {i?98.0:62.0,12};
    case PartKind::PowerSupply:case PartKind::AnalogSource:return {i?110.0:50.0,102};
    case PartKind::Voltmeter:return {i?105.0:55.0,186};
    case PartKind::SevenSegment:{const int half=static_cast<int>((names.size()+1)/2);return {i<half?22.0:138.0,28.0+(i%half)*20};}
    default:if(names.size()>2){int half=(names.size()+1)/2;return {i<half?0.:160.,35.+(i%half)*22};}return {i?160.0:0.0,58};
    }
}
// Shared scene scale: six scene units per millimetre. Dimensions describe
// representative modules, not an interchangeable manufacturer footprint.
QTransform physicalTransform(const CircuitPart& part){
    if(poweredBoard(part))return {};
    QSizeF drawing(1,1),millimetres(1,1);
    switch(part.kind){
    case PartKind::RotaryEncoder:drawing={160,160};millimetres={15,15};break;
    case PartKind::Keypad:drawing={160,170};millimetres={70,77};break;
    case PartKind::Keyboard:drawing={400,140};millimetres={200,70};break;
    case PartKind::InfraredRemote:drawing={90,155};millimetres={45,140};break;
    case PartKind::NfcTestCard:drawing={160,120};millimetres={85.6,54};break;
    case PartKind::NfcReader:drawing={160,160};millimetres={43,41};break;
    case PartKind::RadioTransceiver:drawing={160,160};millimetres={18,29};break;
    case PartKind::RadioTransmitter:drawing={160,160};millimetres={19,19};break;
    case PartKind::RadioReceiver:drawing={160,160};millimetres={30,14};break;
    case PartKind::InfraredReceiver:drawing={160,160};millimetres={7,9};break;
    case PartKind::ServoMotor:case PartKind::DcMotor:drawing={160,130};millimetres={25,20};break;
    case PartKind::AndGate:case PartKind::NandGate:case PartKind::OrGate:case PartKind::NorGate:case PartKind::XorGate:case PartKind::XnorGate:case PartKind::NotGate:drawing={160,130};millimetres={18,14};break;
    case PartKind::CharacterLcd:case PartKind::I2cCharacterLcd:drawing={6,6};millimetres={1,1};break;
    case PartKind::Note:case PartKind::Junction:case PartKind::NetLabel:drawing={6,6};millimetres={1,1};break;
    case PartKind::Esp8266:{const auto id=BoardProfile::get(part.model).id;
        if(id=="arduino-uno"){drawing={300,220};millimetres={68.58,53.34};}
        else if(id=="arduino-nano"){drawing={100,272};millimetres={18,45};}
        else{drawing={150,artBounds(part).height()-24};millimetres=id=="esp8266"?QSizeF(31,58):id=="esp32-s3"?QSizeF(25.4,62.74):QSizeF(27.94,48.26);if(id=="rp2040")millimetres={21,51};else if(id=="stm32f103")millimetres={22.86,53.34};else if(id=="esp32-c3")millimetres={18,35};else if(id=="esp32-c6")millimetres={25.4,58};}break;}
    case PartKind::Capacitor:case PartKind::Inductor:drawing={100,130};millimetres={8,12};break;
    case PartKind::NpnTransistor:case PartKind::PnpTransistor:drawing={100,130};millimetres={5,10};break;
    case PartKind::NMosfet:case PartKind::PMosfet:drawing={100,130};millimetres={10,20};break;
    case PartKind::OpAmp:drawing={160,130};millimetres={18,14};break;
    case PartKind::Ground:drawing={120,100};millimetres={15,12};break;
    case PartKind::FunctionGenerator:case PartKind::Oscilloscope:case PartKind::LogicAnalyzer:case PartKind::CurrentProbe:drawing={168,150};millimetres={45,40};break;
    case PartKind::PiezoBuzzer:drawing={100,100};millimetres={12,12};break;
    case PartKind::Speaker:drawing={100,100};millimetres={15,11};break;
    case PartKind::TemperatureSensor:drawing={64,70};millimetres={4.6,5};break;
    case PartKind::BatteryAA:drawing={80,125};millimetres={14.5,50.5};break;
    case PartKind::ToggleSwitch:drawing={100,75};millimetres={12,8};break;
    case PartKind::Oled:drawing={292,CircuitDocument::displayHeight(part)==32?136.0:204.0};millimetres=CircuitDocument::displayHeight(part)==32?QSizeF(38,12):QSizeF(27.3,27.8);break;
    case PartKind::Breadboard30:drawing={570,260};millimetres={83.8,35.6};break;
    case PartKind::InfraredLed:case PartKind::Led:case PartKind::RGBLed:drawing={60,66};millimetres={5,8.6};break;
    case PartKind::Resistor:drawing={160,30};millimetres={12.7,2.3};break;
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
    case PartKind::Voltmeter:drawing={98,196};millimetres={75,150};break;
    case PartKind::SevenSegment:drawing={84,116};millimetres={12.7,19};break;
    }
    return QTransform::fromScale(6*millimetres.width()/drawing.width(),6*millimetres.height()/drawing.height());
}
}
QRectF ComponentDesign::bounds(const CircuitPart& part){if(part.kind==PartKind::Module){const auto& d=ModuleRegistry::get(part.model);return {-8,-8,(d.implementation=="ws2812"?5*std::clamp(int(parameter(part,"pixels",8)),1,64):d.width)*6+16,d.height*6+16};}if(part.kind==PartKind::Esp8266&&part.model=="arduino-uno")return QRectF(-6.4*6,0,(68.58+6.4)*6,53.34*6).adjusted(-7,-7,7,7);if(part.kind==PartKind::Esp8266&&(part.model=="esp32"||part.model=="esp32-s3")){const bool s3=part.model=="esp32-s3";const double w=s3?25.4:27.94,h=s3?62.74:48.26,overhang=s3?6.3:6.04;return QRectF(0,-overhang*6,w*6,(h+overhang+.8)*6).adjusted(-7,-7,7,7);}if(auto f=PhysicalParts::package(part))return PhysicalParts::bounds(*f);return physicalTransform(part).mapRect(artBounds(part)).adjusted(-7,-7,7,7);}
QPointF ComponentDesign::pinPosition(const CircuitPart& part,const std::string& pin){if(part.kind==PartKind::Module){const auto& d=ModuleRegistry::get(part.model);if(d.implementation=="ws2812"){const double w=30*std::clamp(int(parameter(part,"pixels",8)),1,64);if(pin=="DIN")return {0,30};if(pin=="DOUT")return {w,30};if(pin=="VCC")return {0,15};if(pin=="GND")return {0,45};}const auto position=d.pinPositions.find(pin);if(position!=d.pinPositions.end())return {position->second.first*6,position->second.second*6};const auto it=std::find(d.pins.begin(),d.pins.end(),pin);if(it==d.pins.end())throw std::invalid_argument("Unknown module pin");const int index=int(it-d.pins.begin()),half=int((d.pins.size()+1)/2);const bool right=index>=half;const int row=right?index-half:index,count=right?int(d.pins.size())-half:half;return {right?(d.implementation=="ws2812"?5*std::clamp(int(parameter(part,"pixels",8)),1,64):d.width)*6:0.,d.height*6*(row+1)/(count+1)};}if(auto f=PhysicalParts::package(part)){const auto names=CircuitDocument::pins(part);auto i=std::find(names.begin(),names.end(),pin)-names.begin();if(i>=f->pins.size())throw std::invalid_argument("Unknown package pin");return f->pins[i]*6;}if(poweredBoard(part))return railBoardPin(pin);if(part.kind==PartKind::Oled){const auto pins=CircuitDocument::pins(part);const auto i=std::find(pins.begin(),pins.end(),pin)-pins.begin();const bool small=CircuitDocument::displayHeight(part)==32;const double width=small?38:27.3,height=small?12:27.8;return {6*((width-7.62)/2+i*2.54),6*(height-1.3)};}if(part.kind==PartKind::Breadboard30){const int column=std::stoi(pin.substr(1))-1,row=pin[0]-'a';return {6*(3.5+column*2.54),6*(5.0+row*2.54+(row>=5?5.08:0))};}return physicalTransform(part).map(artPin(part,pin));}
static void paintPhysicalBoard(QPainter& p,const CircuitPart& part,const CircuitState& state,bool selected){
    const auto profile=BoardProfile::get(part.model);const bool uno=profile.id=="arduino-uno",nano=profile.id=="arduino-nano";
    const double width=uno?68.58:nano?18:profile.id=="esp8266"?31:profile.id=="esp32"?27.94:25.4,height=uno?53.34:nano?45:profile.id=="esp8266"?58:profile.id=="esp32-s3"?62.74:48.26;
    p.save();p.scale(6,6);p.setRenderHint(QPainter::Antialiasing);
    auto box=[&](QRectF r,QColor color,double radius=.4){p.setPen(QPen(color.darker(130),.12));p.setBrush(color);p.drawRoundedRect(r,radius,radius);};
    auto circle=[&](QPointF at,double radius,QColor color){p.setPen(Qt::NoPen);p.setBrush(color);p.drawEllipse(at,radius,radius);};
    auto words=[&](QRectF r,QString text,double size=1.1){PhysicalParts::text(p,r,text,Qt::white,size);};
    auto hole=[&](QPointF at){circle(at,1.6,QColor("#b5a37b"));circle(at,1.1,QColor("#e4e8e7"));};
    if(uno){QPainterPath outline;outline.moveTo(1,0);for(auto point:{QPointF(64.516,0),QPointF(66.04,1.524),QPointF(66.04,12.954),QPointF(68.58,15.494),QPointF(68.58,48.26),QPointF(66.04,50.8),QPointF(66.04,52.34),QPointF(65.04,53.34),QPointF(1,53.34),QPointF(0,52.34),QPointF(0,1)})outline.lineTo(point);outline.closeSubpath();p.setPen(QPen(QColor("#1f5f79"),.15));p.setBrush(QColor("#317b99"));p.drawPath(outline);}else box({0,0,width,height},profile.arduino()?QColor("#287a90"):QColor("#3e494d"),.8);
    auto connector=[&](QRectF r){box(r,QColor("#c4ced1"),.5);box(r.adjusted(.8,.8,-.8,-r.height()*.4),QColor("#415058"),.15);};
    if(uno){connector({-6.4,9.2,16,12});box({-2.2,39.5,12,11},QColor("#26353c"),.8);circle({-1.8,44.96},3.2,QColor("#101a20"));box({28.7,33.4,35.3,7.1},QColor("#223239"),.5);words({31,34.5,30,4.5},"ATmega328P",1.7);for(int i=0;i<14;++i){box({29.35+i*2.54,32.4,1,1},QColor("#c4ced1"),.1);box({29.35+i*2.54,40.5,1,1},QColor("#c4ced1"),.1);}box({16.94,15.67,6,6},QColor("#223239"));words({15.5,22,9,2.5},"USB",1.1);box({4.2,2.3,4.3,3.2},QColor("#bd6963"));circle({6.35,3.94},.9,QColor("#5b666b"));box({14.4,25.2,9,4},QColor("#bdc8c8"),1.6);box({39.8,27.5,3,2},QColor("#bdc8c8"),.5);words({28,15,34,8},"UNO R3",5);words({28,24,34,3},"5 V · ATmega328P",1.4);for(auto point:{QPointF(15.24,2.54),QPointF(66.04,17.78),QPointF(13.97,50.8),QPointF(66.04,45.72)})hole(point);}
    else if(nano){connector({5.8,-1,6.4,6});box({6.3,18.5,5.4,5.4},QColor("#223239"),.25);words({6.5,19.8,5,2.6},"328P",1.2);box({6,29,6,2.5},QColor("#bfc9ca"),.8);box({7,8,4,2.5},QColor("#b7bfc2"));circle({9,9.2},.8,QColor("#cb7773"));words({5.5,12,7,3},"NANO",1.6);words({5.5,35,7,3},"5 V",1.4);}
    else{
        const bool s3=profile.id=="esp32-s3",devkit=profile.id=="esp32"||s3;
        if(devkit){const double overhang=s3?6.3:6.04;box({width/2-9,-overhang,18,25.5},QColor("#285343"),.15);box({width/2-8.1,.6,16.2,18.5},QColor("#bec8c9"),.3);PhysicalParts::text(p,{width/2-7.5,7,15,5},s3?"ESP32-S3":"ESP32",QColor("#3b4c53"),1.8);p.setPen(QPen(QColor("#c5af79"),.3));QPainterPath antenna;antenna.moveTo(width/2-7.5,-1);for(int i=0;i<5;++i){double x=width/2-7.5+i*3;antenna.lineTo(x,-1);antenna.lineTo(x,-overhang+1);antenna.lineTo(x+1.5,-overhang+1);antenna.lineTo(x+1.5,-1);}p.setBrush(Qt::NoBrush);p.drawPath(antenna);}
        else{box({width*.22,11,width*.56,19},QColor("#bcc6c8"));PhysicalParts::text(p,{width*.23,17,width*.54,6},"ESP8266",QColor("#3b4c53"),2);box({width*.22,3,width*.56,7},QColor("#254d40"));p.setPen(QPen(QColor("#c5af79"),.35));QPainterPath antenna;antenna.moveTo(width*.27,8.5);for(int i=0;i<5;++i){double x=width*.27+i*width*.09;antenna.lineTo(x,8.5);antenna.lineTo(x,4);antenna.lineTo(x+width*.045,4);antenna.lineTo(x+width*.045,8.5);}p.setBrush(Qt::NoBrush);p.drawPath(antenna);}
        if(s3){connector({2.3,height-6,7.4,6.8});connector({15.7,height-6,7.4,6.8});}else connector({width/2-3.7,height-5.2,7.4,6});
        box({width/2-2.5,height-(s3?23.:18.),5,4},QColor("#223239"));for(double x:{width*.22,width*.68}){const double y=height-(s3?15.:9.);box({x,y,3.3,2.3},QColor("#c3c9ca"));circle({x+1.65,y+1.15},.8,QColor("#34484e"));}words({width*.22,height-(s3?29.:23.),width*.56,3},profile.id=="esp8266"?"NodeMCU":s3?"DevKitC-1":"DevKitC",1.7);if(!devkit){hole({width*.16,1.8});hole({width*.84,1.8});hole({width*.16,height-1.8});hole({width*.84,height-1.8});}
    }
    if(!uno){const int rows=nano||profile.id=="esp8266"?15:profile.id=="esp32-s3"?22:19;const double top=nano?24*45./272:profile.id=="esp8266"?(height-14*2.54)/2:profile.id=="esp32-s3"?1.4:(height-(rows-1)*2.54)/2;const double left=nano?8*18./100:profile.id=="esp8266"?8*31./150:1.27;const double right=left+(nano?15.24:profile.id=="esp8266"?27.94:profile.id=="esp32"?25.4:22.86);for(int row=0;row<rows;++row)for(double x:{left,right}){box({x-1.02,top+row*2.54-1.02,2.04,2.04},QColor("#2c3c43"),.12);circle({x,top+row*2.54},.38,QColor("#768079"));}}
    if(uno){auto socket=[&](QPointF at){box({at.x()-1.02,at.y()-1.02,2.04,2.04},QColor("#2c3c43"),.12);circle(at,.38,QColor("#768079"));};for(int i=0;i<10;++i)socket({18.796+i*2.54,2.54});for(int i=0;i<8;++i){socket({45.72+i*2.54,2.54});socket({27.94+i*2.54,50.8});}for(int i=0;i<6;++i)socket({50.8+i*2.54,50.8});}
    const auto names=CircuitDocument::pins(part);
    std::vector<QPointF> labeled;for(const auto& name:names){const auto at=ComponentDesign::pinPosition(part,name)/6;if(std::any_of(labeled.begin(),labeled.end(),[&](auto other){return QLineF(other,at).length()<.01;}))continue;labeled.push_back(at);box({at.x()-1.02,at.y()-1.02,2.04,2.04},QColor("#223239"),.15);auto text=QString::fromStdString(name);text.replace("GPIO","");if(name=="GND2"||name=="GND3")text="GND";if(name=="3V3_2")text="3V3";
        if(profile.id=="esp8266"&&name.starts_with("GPIO")){int gpio=profile.pinNumber(name);if(gpio==3)text="RX";else if(gpio==1)text="TX";else for(std::size_t i=0;i<Esp8266Board::nodeMcuGpio.size();++i)if(gpio==Esp8266Board::nodeMcuGpio[i])text="D"+QString::number(i);}
        if(profile.supportsPwm(profile.pinNumber(name)))text.prepend("~");
        if(uno){p.save();p.translate(at.x(),at.y()<10?at.y()+1.3:at.y()-1.5);p.rotate(-90);words(at.y()<10?QRectF(-3.5,-.9,3.5,1.8):QRectF(0,-.9,6,1.8),text,1.05);p.restore();}
        else{const double labelWidth=nano||profile.id=="esp32-s3"?2.6:profile.id=="esp8266"?4.5:2.8;words({at.x()<width/2?at.x()+1.2:at.x()-1.2-labelWidth,at.y()-.85,labelWidth,1.7},text,nano?.85:profile.id=="esp32-s3"?.9:1.0);}
    }
    const bool lit=state.level({part.id,profile.pinName(profile.builtinLed)})==(profile.arduino()?Level::High:Level::Low);box({uno?26.94:nano?width/2-1:width*.22,uno?10.676:height-(nano?11.:profile.id=="esp32-s3"?30.:22.),2,1},lit?QColor(uno?"#e7c959":"#68d899"):QColor("#375d51"),.1);
    if(selected){p.setPen(QPen(QColor("#14b8a6"),.25,Qt::DashLine));p.setBrush(Qt::NoBrush);p.drawRoundedRect(QRectF(-.7,-.7,width+1.4,height+1.4),1,1);}p.restore();
}
void ComponentDesign::paint(QPainter& p,const CircuitPart& part,const CircuitState& state,const Frame& frame,double brightness,bool selected){
        if(part.kind==PartKind::Module){
        const auto& d=ModuleRegistry::get(part.model);const double w=(d.implementation=="ws2812"?5*std::clamp(int(parameter(part,"pixels",8)),1,64):d.width)*6,h=d.height*6;p.save();p.setRenderHint(QPainter::Antialiasing);p.setPen(QPen(selected?QColor("#11a698"):QColor("#254c3e"),selected?2:1));p.setBrush(QColor(d.implementation=="mfrc522"?"#185d8b":"#236c50"));if(d.implementation!="dht22"&&d.implementation!="ds18b20")p.drawRoundedRect(QRectF(0,0,w,h),3,3);else{p.setPen(QPen(QColor("#aeb5ba"),2));for(const auto& pin:d.pins){const auto at=ComponentDesign::pinPosition(part,pin);p.drawLine(at,QPointF(at.x(),h*.45));}}
        if(d.implementation=="hcsr04"){for(double x:{w*.25,w*.75}){p.setBrush(QColor("#bbc3ca"));p.setPen(QPen(QColor("#7e8991"),2));p.drawEllipse(QPointF(x,h*.48),h*.37,h*.37);p.setBrush(QColor("#444c52"));p.drawEllipse(QPointF(x,h*.48),h*.28,h*.28);p.setPen(QPen(QColor("#8a939c"),1));for(int n=-3;n<=3;++n)p.drawLine(QPointF(x-h*.22,h*.48+n*5),QPointF(x+h*.22,h*.48+n*5));}}
        else if(d.implementation=="dht22"){p.setBrush(QColor("#ece9df"));p.setPen(QPen(QColor("#b7b9b0"),1));p.drawRoundedRect(QRectF(w*.06,0,w*.88,h*.86),4,4);p.setPen(QPen(QColor("#787c75"),3));for(int y=12;y<int(h*.75);y+=9)p.drawLine(QPointF(w*.2,y),QPointF(w*.8,y));}
        else if(d.implementation=="ds18b20"){p.setBrush(QColor("#252b30"));p.setPen(QPen(QColor("#0d1116"),1));p.drawRoundedRect(QRectF(0,0,w,h*.51),w*.3,w*.2);}
        else if(d.implementation=="ws2812"){const int count=std::clamp(int(parameter(part,"pixels",8)),1,64);const auto found=state.pixels.find(part.id);for(int i=0;i<count;++i){const double cell=w/count;p.setBrush(QColor("#e9e8d8"));p.setPen(QPen(QColor("#c4c6bc"),1));p.drawRect(QRectF(i*cell+2,h*.12,std::max(2.,cell-4),h*.72));std::uint32_t color=0;if(found!=state.pixels.end()&&i<int(found->second.size()))color=found->second[i];p.setBrush(color?QColor::fromRgb(color):QColor("#555e61"));p.setPen(Qt::NoPen);p.drawEllipse(QPointF((i+.5)*cell,h*.48),std::max(1.,cell*.2),h*.2);}}
        else if(d.implementation=="l298n"){p.setBrush(QColor("#161b1e"));p.setPen(QPen(QColor("#6b767c"),2));p.drawRect(QRectF(w*.17,h*.12,w*.66,h*.5));for(int x=int(w*.2);x<int(w*.8);x+=10)p.drawLine(QPointF(x,h*.14),QPointF(x,h*.6));p.setBrush(QColor("#2f7a9c"));p.drawRect(QRectF(w*.14,h*.72,w*.72,h*.15));}
        else{if(d.implementation=="mfrc522"){p.setPen(QPen(QColor("#d9b16a"),2));p.setBrush(Qt::NoBrush);for(int n=0;n<4;++n)p.drawRoundedRect(QRectF(10+n*7,10+n*7,w-20-n*14,h*.58-n*14),8,8);}p.setPen(QPen(QColor("#111b1b"),1));p.setBrush(QColor("#24292c"));p.drawRoundedRect(QRectF(w*.28,h*.35,w*.44,h*.28),2,2);for(int n=0;n<4;++n){p.setBrush(QColor("#c8ba91"));p.drawRect(QRectF(w*.12,h*.24+n*9,w*.1,4));}}
        for(const auto& pin:d.pins){const auto at=ComponentDesign::pinPosition(part,pin);p.setPen(QPen(QColor("#b7a66b"),3));p.setBrush(QColor("#22282d"));p.drawEllipse(at,3,3);}
        if(d.implementation!="dht22"&&d.implementation!="ds18b20"&&d.implementation!="ws2812"){p.setPen(QColor("#e1eee4"));QFont font("Sans Serif");font.setPixelSize(8);p.setFont(font);p.drawText(QRectF(5,h*.82,w-10,h*.15),Qt::AlignCenter,QString::fromStdString(d.label));}p.restore();return;
    }
    if(part.kind==PartKind::Esp8266&&(part.model.empty()||part.model=="esp8266"||part.model=="esp32"||part.model=="esp32-s3"||part.model=="arduino-uno"||part.model=="arduino-nano")){paintPhysicalBoard(p,part,state,selected);return;}
    if(PhysicalParts::paint(p,part,state,frame,brightness,selected))return;
    if(part.kind==PartKind::Breadboard30&&!poweredBoard(part)){
        p.save();p.scale(6,6);p.setPen(QPen(QColor("#c5c3bb"),.15));p.setBrush(QColor("#eeeae1"));p.drawRoundedRect(QRectF(0,0,83.8,35.6),1,1);p.setPen(Qt::NoPen);p.setBrush(QColor("#c9c7be"));p.drawRoundedRect(QRectF(1,18.2,81.8,2.54),.4,.4);
        for(int i=0;i<30;++i)if(i==0||(i+1)%5==0)PhysicalParts::text(p,{2.3+i*2.54,1,2.4,2},QString::number(i+1),QColor("#7e817a"),.9);
        for(int row=0;row<10;++row)PhysicalParts::text(p,{.2,4.+row*2.54+(row>=5?5.08:0),1.4,2},QString(QChar('a'+row)),QColor("#7e817a"),.9);
        p.restore();return;
    }
    p.save();p.setTransform(physicalTransform(part),true);p.setRenderHint(QPainter::Antialiasing);
    if(selected){p.setPen(QPen(QColor("#14b8a6"),1.5,Qt::DashLine));p.setBrush(Qt::NoBrush);p.drawRoundedRect(artBounds(part).adjusted(3,3,-3,-3),6,6);}
    const auto current=[&](const std::string& channel){auto it=state.ledCurrent.find(part.id+channel);return it==state.ledCurrent.end()?0.0:it->second;};
    switch(part.kind){
    case PartKind::Esp8266:board(p,part,state);break;
    case PartKind::RotaryEncoder:{body(p,{12,18,136,120},QColor("#b7c1c5"),4);p.setBrush(QColor("#273741"));p.setPen(QPen(QColor("#667985"),3));p.drawEllipse(QPointF(80,70),48,48);p.setPen(QPen(QColor("#e5e7eb"),4));p.drawLine(QPointF(80,70),QPointF(80+35*std::sin(parameter(part,"phase",0)*1.5708),70-35*std::cos(parameter(part,"phase",0)*1.5708)));break;}
    case PartKind::Keypad:{body(p,{0,0,160,160},QColor("#334a59"),8);const QStringList names{"1","2","3","A","4","5","6","B","7","8","9","C","*","0","#","D"};const auto mask=uint32_t(parameter(part,"keys",0));for(int k=0;k<16;++k){QRectF r(9+(k%4)*37,9+(k/4)*37,31,31);body(p,r,mask&(1u<<k)?QColor("#20c1ad"):QColor(k%4==3?"#cf6265":"#e2e9ed"),4);label(p,r,names[k],QColor("#263b45"),14);}break;}
    case PartKind::Keyboard:{body(p,{0,0,400,140},QColor("#263741"),5);const auto mask=uint32_t(parameter(part,"keys",0));int white=0;auto isBlack=[](int n){n%=12;return n==1||n==3||n==6||n==8||n==10;};for(int k=0;k<25;++k)if(!isBlack(k)){QRectF r(13+white++*25,28,24,100);body(p,r,mask&(1u<<k)?QColor("#20c1ad"):QColor("#f1f3f5"),1);}white=0;for(int k=0;k<25;++k){if(!isBlack(k)){++white;continue;}body(p,{13+white*25-7.,28,14,62},mask&(1u<<k)?QColor("#20c1ad"):QColor("#15202b"),1);}label(p,{10,4,380,20},"25 KEY · MATRIX / MIDI",QColor("#d0dce1"),8);break;}
    case PartKind::InfraredRemote:{body(p,{35,0,90,155},QColor("#2d3540"),8);label(p,{40,130,80,16},"NEC · TV",Qt::white,10);p.setBrush(part.pressed?QColor("#fb923c"):QColor("#dc4545"));p.setPen(Qt::NoPen);p.drawEllipse(QPointF(60,24),10,10);for(int row=0;row<4;++row)for(int col=0;col<3;++col){p.setBrush(QColor("#c1c8d0"));p.drawRoundedRect(QRectF(45+col*23,48+row*18,16,11),3,3);}break;}
    case PartKind::NfcTestCard:{body(p,{0,15,160,120},part.pressed?QColor("#225f9b"):QColor("#738194"),8);body(p,{15,43,30,28},QColor("#d6b35e"),4);label(p,{50,30,98,24},"NFC TEST",Qt::white,16);label(p,{10,90,140,25},"SYNTHETIC TAG",Qt::white,10);p.setPen(QPen(Qt::white,2));for(int i=0;i<3;++i)p.drawArc(QRectF(102-i*6,55-i*6,24+i*12,24+i*12),-60*16,120*16);break;}
    case PartKind::InfraredReceiver:{for(const auto& pin:CircuitDocument::pins(part)){auto point=artPin(part,pin);lead(p,point,{point.x(),90});label(p,{point.x()-14,122,28,12},QString::fromStdString(pin),QColor("#334155"),6);}body(p,{23,13,114,93},QColor("#242729"),6);p.setBrush(QColor("#373c40"));p.setPen(Qt::NoPen);p.drawEllipse(QPointF(80,58),34,36);label(p,{35,90,90,12},"38 kHz",QColor("#c3c8cc"),8);break;}
    case PartKind::RadioTransceiver:{body(p,{7,0,146,155},QColor("#215748"),2);p.setPen(QPen(QColor("#d6bd72"),3));QPainterPath antenna;antenna.moveTo(145,45);antenna.lineTo(135,45);antenna.lineTo(135,10);antenna.lineTo(113,10);antenna.lineTo(113,35);antenna.lineTo(91,35);antenna.lineTo(91,10);antenna.lineTo(69,10);antenna.lineTo(69,35);antenna.lineTo(47,35);antenna.lineTo(47,10);antenna.lineTo(23,10);antenna.lineTo(23,49);p.setBrush(Qt::NoBrush);p.drawPath(antenna);chip(p,{56,66,48,31},6,"nRF24");body(p,{19,67,23,16},QColor("#b8bec2"),2);for(int i=0;i<5;++i){p.setBrush(QColor("#b9a274"));p.drawRect(QRectF(112,61+i*10,13,5));}for(const auto& name:CircuitDocument::pins(part)){auto point=artPin(part,name);p.setBrush(QColor("#b8a56a"));p.setPen(QPen(QColor("#161e23"),3));p.drawEllipse(point,4,4);label(p,{point.x()-17,point.y()-15,34,12},QString::fromStdString(name),Qt::white,6);}break;}
    case PartKind::RadioTransmitter:case PartKind::RadioReceiver:{const bool tx=part.kind==PartKind::RadioTransmitter;body(p,{8,15,144,103},QColor("#28714c"),2);for(const auto& name:CircuitDocument::pins(part)){auto point=artPin(part,name);lead(p,point,{point.x(),113});label(p,{point.x()-18,119,36,12},QString::fromStdString(name),QColor("#334155"),6);}if(tx){body(p,{37,35,52,38},QColor("#bec6c9"),12);label(p,{34,42,58,20},"433",QColor("#4b555b"),12);chip(p,{99,61,21,27},3,"TX");}else{chip(p,{62,62,46,29},5,"RX");p.setPen(QPen(QColor("#c28c4a"),3));for(int i=0;i<5;++i){p.drawArc(QRectF(26+i*8,33,13,17),0,300*16);p.drawArc(QRectF(91+i*8,33,13,17),0,300*16);}}break;}
    case PartKind::NfcReader:{for(const auto& name:CircuitDocument::pins(part)){auto point=artPin(part,name);lead(p,point,{point.x()<80?25.:135.,point.y()});label(p,{point.x()<80?0.:136.,point.y()-16,24,14},QString::fromStdString(name),QColor("#334155"),6);}body(p,{23,10,114,135},QColor("#a03639"),4);p.setPen(QPen(QColor("#e0b960"),1.5));p.setBrush(Qt::NoBrush);for(int i=0;i<4;++i)p.drawRoundedRect(QRectF(30+i*5,18+i*5,100-i*10,112-i*10),7,7);chip(p,{57,56,46,40},6,"PN532");break;}

    case PartKind::ServoMotor:case PartKind::DcMotor:{for(const auto& name:CircuitDocument::pins(part)){auto point=artPin(part,name);lead(p,point,{80,90});label(p,{point.x()<80?0.:130.,point.y()-18,30,14},QString::fromStdString(name));}const bool servo=part.kind==PartKind::ServoMotor;body(p,{25,15,110,100},servo?QColor("#28639c"):QColor("#b4bbc0"),servo?3:20);p.setBrush(QColor("#e4e7eb"));p.setPen(QPen(QColor("#52616c"),1));p.drawEllipse(QPointF(80,50),20,20);double value=state.partValues.contains(part.id)?state.partValues.at(part.id):0;if(servo){p.save();p.translate(80,50);p.rotate(value-90);p.setPen(QPen(QColor("#e5e7eb"),9,Qt::SolidLine,Qt::RoundCap));p.drawLine(QPointF(-30,0),QPointF(30,0));p.restore();}label(p,{35,85,90,20},QString::number(value,'f',1)+(servo?"°":" rpm"));break;}

    case PartKind::AndGate:case PartKind::NandGate:case PartKind::OrGate:case PartKind::NorGate:case PartKind::XorGate:case PartKind::XnorGate:case PartKind::NotGate:{for(const auto& name:CircuitDocument::pins(part)){auto point=artPin(part,name);lead(p,point,{point.x()<80?32.:128.,point.y()});label(p,{point.x()<80?0.:135.,point.y()-16,25,14},QString::fromStdString(name));}body(p,{30,15,100,115},QColor("#263844"),3);label(p,{40,48,80,28},QString::fromStdString(componentSpec(part.kind).label),Qt::white,18);break;}

    case PartKind::Capacitor:{
        lead(p,{60,80},{60,136});lead(p,{100,80},{100,136});body(p,{43,18,74,82},QColor("#277c9c"),8);p.setPen(Qt::NoPen);p.setBrush(QColor("#a8b7c5"));p.drawEllipse(QRectF(43,12,74,20));p.setPen(QPen(QColor("#66778a"),2));p.drawLine(QPointF(53,22),QPointF(107,22));label(p,{49,46,62,30},QString::number(part.value)+" µF",Qt::white,10);break;}
    case PartKind::Inductor:{
        lead(p,{60,70},{60,136});lead(p,{100,70},{100,136});body(p,{40,25,80,72},QColor("#3c454e"),12);p.setPen(QPen(QColor("#bc7538"),6));for(int i=0;i<6;++i)p.drawEllipse(QRectF(42+i*10,30,22,60));label(p,{35,102,90,16},QString::number(part.value)+" mH");break;}
    case PartKind::NpnTransistor:case PartKind::PnpTransistor:case PartKind::NMosfet:case PartKind::PMosfet:{
        const bool mos=part.kind==PartKind::NMosfet||part.kind==PartKind::PMosfet;
        for(const auto& name:CircuitDocument::pins(part)){auto point=artPin(part,name);lead(p,{point.x(),70},point);label(p,{point.x()-12,115,24,15},QString::fromStdString(name));}
        if(mos){body(p,{40,8,80,40},QColor("#acb5bd"),2);p.setBrush(QColor("#e4e8eb"));p.drawEllipse(QPointF(80,25),7,7);}body(p,{38,mos?40.0:20.0,84,58},QColor("#2d343a"),mos?2:14);label(p,{42,43,76,24},part.kind==PartKind::NpnTransistor?"NPN":part.kind==PartKind::PnpTransistor?"PNP":part.kind==PartKind::NMosfet?"N-MOS":"P-MOS",Qt::white,11);break;}
    case PartKind::OpAmp:{
        p.setPen(QPen(metal,3));for(const auto& name:CircuitDocument::pins(part))p.drawLine(artPin(part,name),QPointF(80,65));QPainterPath triangle;triangle.moveTo(25,20);triangle.lineTo(135,65);triangle.lineTo(25,110);triangle.closeSubpath();p.setBrush(QColor("#263844"));p.setPen(QPen(QColor("#97a6b7"),2));p.drawPath(triangle);label(p,{30,35,22,22},"+",Qt::white,16);label(p,{30,74,22,22},"−",Qt::white,16);label(p,{55,53,50,24},"OP",Qt::white,13);break;}
    case PartKind::Ground:{
        p.setPen(QPen(QColor("#526c79"),4));p.drawLine(QPointF(80,26),QPointF(80,60));for(int i=0;i<3;++i)p.drawLine(QPointF(45+i*12,60+i*13),QPointF(115-i*12,60+i*13));label(p,{30,100,100,15},"GND · 0 V");break;}
    case PartKind::FunctionGenerator:case PartKind::Oscilloscope:case PartKind::LogicAnalyzer:case PartKind::CurrentProbe:{
        body(p,{2,8,156,122},QColor("#c4ccd2"),6);body(p,{12,25,136,65},QColor("#132938"),3);
        const bool gen=part.kind==PartKind::FunctionGenerator;label(p,{12,10,136,14},gen?"FUNCTION GENERATOR":part.kind==PartKind::Oscilloscope?"OSCILLOSCOPE":part.kind==PartKind::LogicAnalyzer?"LOGIC ANALYZER":"CURRENT PROBE",QColor("#283a48"),7);
        // Front panel markings; acquired traces are displayed in the instrument dock.
        label(p,{16,34,128,44},gen?QString::number(part.value)+" Hz":part.kind==PartKind::CurrentProbe?"I  + → −":part.kind==PartKind::LogicAnalyzer?"D0 … D7":"CH A / CH B",QColor("#4ade80"),13);
        for(const auto& name:CircuitDocument::pins(part)){auto point=artPin(part,name);lead(p,{point.x(),122},point);label(p,{point.x()-10,103,20,15},QString::fromStdString(name),QColor("#334155"),6);}break;}

    case PartKind::Oled:{
        const bool small=CircuitDocument::displayHeight(part)==32;const double height=small?136:204;const auto scale=physicalTransform(part);const double screenHeight=244.0*CircuitDocument::displayHeight(part)/128.0*scale.m11()/scale.m22();
        body(p,{0,0,292,height},QColor("#145b89"),5);for(auto point:{QPointF(10,10),QPointF(282,10),QPointF(10,height-20),QPointF(282,height-20)})mountingHole(p,point);
        body(p,{18,19,256,screenHeight+12},QColor("#151d28"),2);p.setPen(Qt::NoPen);p.setBrush(QColor("#02080d"));p.drawRect(QRectF(24,24,244,screenHeight));
        const auto v=state.voltage.find({part.id,"VCC"}),g=state.voltage.find({part.id,"GND"});
        const bool powered=v!=state.voltage.end()&&g!=state.voltage.end()&&v->second&&g->second&&*v->second-*g->second>=3;
        if(powered){auto light=QColor("#87e8ff");light.setAlphaF(brightness);p.setBrush(light);const double sx=244.0/frame.width,sy=screenHeight/CircuitDocument::displayHeight(part);
            for(int y=0;y<std::min(frame.height,CircuitDocument::displayHeight(part));++y)for(int x=0;x<frame.width;++x)if(frame.pixel(x,y))p.drawRect(QRectF(24+x*sx,24+y*sy,sx*0.9,sy*0.9));}
        const double pitch=15.24/scale.m11();header(p,{146-2*pitch,height-15.24/scale.m22(),4*pitch,15.24/scale.m22()});
        if(!small)label(p,{30,height-52,232,16},"SSD1306",Qt::white,7);
        break;}
    case PartKind::Breadboard30:{
        if(poweredBoard(part)){paintRailBoard(p);break;}
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
    case PartKind::InfraredLed:case PartKind::Led:case PartKind::RGBLed:{
        for(const auto& name:CircuitDocument::pins(part)){
            const auto point=artPin(part,name);
            if(part.kind==PartKind::Led&&name=="A"){
                const double x=point.x();lead(p,{x,76},{x,96});lead(p,{x,96},{x+9,104});lead(p,{x+9,104},{x,112});lead(p,{x,112},point);
            }else lead(p,{point.x(),76},point);
        }
        QColor color=QColor(QString::fromStdString(part.color));double light=current("");
        if(part.kind==PartKind::RGBLed){auto channel=[&](const char* name){return std::clamp(current(std::string(":")+name)/0.02,0.0,1.0);};color=QColor::fromRgbF(channel("R"),channel("G"),channel("B"));light=std::max({channel("R"),channel("G"),channel("B")});}
        QPainterPath dome({50,77});dome.lineTo(50,44);dome.cubicTo(50,8,110,8,110,44);dome.lineTo(110,77);dome.closeSubpath();
        p.setBrush(light>1e-5?color.lighter(125):color.darker(115));p.setPen(QPen(color.darker(160),1));p.drawPath(dome);
        body(p,{45,73,70,9},color,2);
        // Flat cathode edge and the anode kink make polarity visible without tooltips.
        p.setPen(QPen(color.darker(180),2));p.drawLine(109,61,109,77);break;}
    case PartKind::PiezoBuzzer:case PartKind::Speaker:{
        const bool piezo=part.kind==PartKind::PiezoBuzzer;
        lead(p,{55,98},{55,138});lead(p,{105,98},{105,138});
        if(piezo){
            p.setPen(Qt::NoPen);p.setBrush(QColor("#303234"));p.drawEllipse(QPointF(80,61),50,50);
            p.setBrush(QColor("#151719"));p.drawEllipse(QPointF(80,61),9,9);
            label(p,{42,87,22,18},"+",QColor("#dedede"),13);
        }else{
            body(p,{30,11,100,100},QColor("#34383b"),12);
            body(p,{36,18,88,84},QColor("#1c2023"),10);
            p.setPen(QPen(QColor("#535b60"),1.2));p.setBrush(QColor("#41494d"));p.drawEllipse(QPointF(80,60),32,37);
            p.setBrush(QColor("#242a2d"));p.drawEllipse(QPointF(80,60),14,16);
            for(int i=0;i<5;++i){p.setPen(QPen(QColor("#161a1c"),2));p.drawLine(48,28+i*16,112,28+i*16);}
            label(p,{35,112,90,12},"8 Ω",QColor("#64748b"),8);
        }break;}
    case PartKind::TemperatureSensor:{
        for(const auto& name:CircuitDocument::pins(part)){const auto point=artPin(part,name);lead(p,{point.x(),75},point);}
        QPainterPath package({48,82});package.lineTo(48,35);package.quadTo(48,16,80,16);package.quadTo(112,16,112,35);package.lineTo(112,82);package.closeSubpath();
        p.setPen(Qt::NoPen);p.setBrush(QColor("#303438"));p.drawPath(package);label(p,{49,35,62,20},"TMP",Qt::white,14);label(p,{49,55,62,18},"36",Qt::white,12);break;}
    case PartKind::BatteryAA:{
        body(p,{40,15,80,125},QColor("#303436"),2);body(p,{48,20,64,115},QColor("#369da4"),1);
        body(p,{67,5,26,10},QColor("#c0c4c6"),1);lead(p,{80,8},{80,15});lead(p,{80,140},{80,145});
        label(p,{47,23,66,18},"+",Qt::white,16);label(p,{47,61,66,24},"AA",Qt::white,18);label(p,{47,88,66,20},"1.5 V",Qt::white,13);label(p,{47,115,66,18},"−",Qt::white,16);break;}
    case PartKind::ToggleSwitch:{
        lead(p,{40,70},{40,118});lead(p,{120,70},{120,118});body(p,{30,27,100,75},QColor("#383d41"),4);
        body(p,{45,34,70,60},part.pressed?QColor("#287d60"):QColor("#24292d"),3);
        label(p,{48,part.pressed?39.0:65.0,64,22},part.pressed?"I":"O",Qt::white,18);break;}
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
        p.setBrush(QColor("#bec3c6"));p.drawEllipse(QPointF(80,59),44,44);
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
        body(p,{31,6,98,196},QColor("#c79731"),6);body(p,{37,11,86,186},QColor("#2c3439"),4);label(p,{47,14,66,10},"DIGITAL MULTIMETER",QColor("#b7c1c3"),4);
        body(p,{43,27,74,31},QColor("#14201c"),1);body(p,{46,30,68,25},QColor("#b1be9d"),0.5);
        const auto a=state.voltage.find({part.id,"+"}),b=state.voltage.find({part.id,"-"});const QString text=a!=state.voltage.end()&&b!=state.voltage.end()&&a->second&&b->second?QString::number(*a->second-*b->second,'f',2):"—";
        label(p,{48,31,64,22},text,QColor("#2d3b2b"),12);label(p,{95,48,15,6},"V DC",QColor("#3b4637"),3);
        dial(p,{80,115},24,-25);
        for(int i=0;i<7;++i){const double angle=(-150+i*50)*3.141592653589793/180;const QPointF point=QPointF(80,115)+QPointF(std::cos(angle),std::sin(angle))*35;label(p,{point.x()-8,point.y()-4,16,8},QStringList{"V DC","200","20","2","Ω","V AC","OFF"}[i],QColor("#c3cacd"),3);}
        terminal(p,{55,186},QColor("#a23737"));terminal(p,{105,186},QColor("#252b2f"));label(p,{62,180,38,12},"V Ω  COM",QColor("#c3cacd"),3);break;}
    case PartKind::SevenSegment:{
        const auto names=CircuitDocument::pins(part);for(const auto& name:names){auto point=artPin(part,name);lead(p,point,{point.x()<80?45.0:115.0,point.y()});}
        body(p,{38,11,84,116},QColor("#292e35"),4);body(p,{44,17,72,104},QColor("#11171e"),2);
        const std::array<QLineF,7> segments{QLineF(59,30,96,30),QLineF(101,35,101,62),QLineF(101,72,101,100),QLineF(58,105,96,105),QLineF(53,72,53,100),QLineF(53,35,53,62),QLineF(59,67,96,67)};
        for(int i=0;i<7;++i){p.setPen(QPen(current(":"+std::string(1,char('a'+i)))>1e-5?QColor("#ff524c"):QColor("#41272c"),6,Qt::SolidLine,Qt::RoundCap));p.drawLine(segments[i]);}
        p.setPen(Qt::NoPen);p.setBrush(current(":dp")>1e-5?QColor("#ff524c"):QColor("#41272c"));p.drawEllipse(QPointF(109,110),3,3);break;}
    }
    p.restore();
}
