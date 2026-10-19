#pragma once
#include "thinkercad/circuit.hpp"
#include <QPainter>
#include <QPainterPath>
#include <QFontMetricsF>
#include <optional>
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>
#include <QStringList>
// Dimensions in millimetres. These are named representative packages, not
// interchangeable footprints for every manufacturer variant of a part.
namespace PhysicalParts {
using namespace thinkercad;
struct Package {QRectF body;std::vector<QPointF> pins;QString name;};
inline std::optional<Package> package(const CircuitPart& p){
    using K=PartKind;if(p.kind==K::Esp8266||p.kind==K::Breadboard30||p.kind==K::Note||p.kind==K::Junction||p.kind==K::NetLabel)return {};Package f;const auto count=CircuitDocument::pins(p).size();
    auto row=[&](QRectF body,double pitch,double y,QString name){f={body,{},name};for(std::size_t i=0;i<count;++i)f.pins.push_back({body.center().x()+(double(i)-(count-1)/2.)*pitch,y});};
    auto dip=[&](int positions,QString name){const double length=(positions/2-1)*2.54+3.0;f={{1.0,0,5.62,length},{},name};for(std::size_t i=0;i<count;++i){int index=int(i);f.pins.push_back(index<positions/2?QPointF(0,1.5+index*2.54):QPointF(7.62,1.5+(positions-1-index)*2.54));}};
    switch(p.kind){
    case K::Ground:f={{-5,0,10,8},{{0,0}},"Ground reference symbol"};break;
    case K::PowerSupply:case K::AnalogSource:f={{0,0,90,70},{{25,58},{65,58}},"Bench source · 90×70 mm case"};break;
    case K::Voltmeter:f={{0,0,75,150},{{20,137},{55,137}},"Handheld meter · 75×150 mm case"};break;
    case K::FunctionGenerator:case K::CurrentProbe:f={{0,0,45,40},{{12,43},{33,43}},"Virtual instrument · 45×40 mm case"};break;
    case K::Oscilloscope:row({0,0,45,40},12.7,43,"Virtual scope · 45×40 mm case");break;
    case K::LogicAnalyzer:row({0,0,45,40},2.54,43,"Virtual analyzer · 2.54 mm header");break;
    case K::BatteryAA:f={{0,0,14.5,50.5},{{7.25,0},{7.25,50.5}},"AA cell · 14.5×50.5 mm"};break;
    case K::Battery9V:f={{0,0,26.5,48.5},{{7.3,3},{19.2,3}},"PP3 battery · 26.5×48.5 mm"};break;
    case K::Battery3V:f={{0,0,20,20},{{-2,10},{22,10}},"CR2032 cell · 20 mm with holder contacts"};break;
    case K::Keypad:row({0,0,70,77},2.54,82,"4×4 membrane keypad · 2.54 mm tail adapter");break;
    case K::Keyboard:row({0,0,200,70},2.54,76,"25-key controller · 2.54 mm matrix adapter");break;
    case K::ServoMotor:row({0,0,23,12.2},2.54,20.32,"Micro servo · 23×12.2 mm, 2.54 mm cable connector");break;
    case K::DcMotor:if(p.model=="with-encoder"){row({0,0,20.4,35.1},2.54,40,"DC motor + 20 P/R encoder · 2.54 mm cable adapter");break;}if(p.model=="vibration"){f={{0,0,10,10},{{2,12.7},{4.54,12.7}},"Coin vibration motor · 10 mm"};break;}f={{0,0,20.4,25.1},{{6.2,28},{14.2,28}},"Miniature DC motor · 20.4×25.1 mm, solder tabs"};break;
    case K::InfraredRemote:f={{0,0,45,140},{},"TV remote · 45×140 mm"};break;
    case K::NfcTestCard:f={{0,0,85.6,53.98},{},"ID-1 test card · 85.6×53.98 mm"};break;
    case K::Oled:row({0,0,CircuitDocument::displayHeight(p)==32?38.:27.3,CircuitDocument::displayHeight(p)==32?12.:27.8},2.54,CircuitDocument::displayHeight(p)==32?10.7:26.5,"SSD1306 module · 2.54 mm");break;
    case K::CharacterLcd:case K::I2cCharacterLcd:{const bool large=p.model=="lcd-20x4";f={{0,0,large?98.:80.,large?60.:36.},{},"HD44780 module · 2.54 mm"};for(std::size_t i=0;i<count;++i)f.pins.push_back({large?10.+i*2.54:8.+i*2.54,2.54});break;}
    case K::InfraredReceiver:row({0,0,6.8,7},2.54,12.7,"IR receiver · formed 2.54 mm leads");break;
    case K::NfcReader:row({0,0,43,41},2.54,39,"PN532 I²C module · 2.54 mm header");break;
    case K::RadioTransceiver:f={{0,0,18,29},{},"nRF24L01 module · 2.54 mm header"};f.pins={{5.19,25.54},{5.19,23},{10.27,23},{12.81,23},{10.27,25.54},{7.73,25.54},{7.73,23},{12.81,25.54}};break;
    case K::RadioTransmitter:row({0,0,19,19},2.54,21,"433 MHz transmitter · 2.54 mm header");break;
    case K::RadioReceiver:row({0,0,30,14},2.54,16,"433 MHz receiver · 2.54 mm header");break;
    case K::Resistor:f={{3.2,-1.15,6.3,2.3},{{0,0},{12.7,0}},"Axial ¼ W · 12.7 mm"};break;
    case K::Diode:f={{3.75,-1.35,5.2,2.7},{{0,0},{12.7,0}},"DO-41 · 12.7 mm"};break;
    case K::ReedSwitch:f={{2.54,-1.25,14,2.5},{{0,0},{20.32,0}},"Reed · 20.32 mm"};break;
    case K::TiltSwitch:f={{2.62,-2.25,10,4.5},{{0,0},{15.24,0}},"Tilt · 15.24 mm"};break;
    case K::Led:case K::InfraredLed:row({-.23,0,3,5},2.54,7.62,"LED 3 mm · 2.54 mm");break;
    case K::RGBLed:row({0,0,5,8.6},2.54,12.7,"RGB LED 5 mm · formed 2.54 mm R/K/G/B leads");std::swap(f.pins[1],f.pins[3]);std::swap(f.pins[1],f.pins[2]);break;
    case K::Capacitor:row({0,0,6.3,11},2.54,15,"Radial 6.3 mm · 2.54 mm");break;
    case K::Inductor:row({0,0,8,8},5.08,12,"Radial 8 mm · 5.08 mm");break;
    case K::Photoresistor:case K::Thermistor:row({0,0,5,5},2.54,10,"Disc 5 mm · 2.54 mm");break;
    case K::NpnTransistor:case K::PnpTransistor:case K::TemperatureSensor:row({0,0,4.6,4.6},2.54,10,"TO-92 · formed 2.54 mm leads");break;
    case K::NMosfet:case K::PMosfet:row({0,0,10.16,15.0},2.54,21,"TO-220 · 2.54 mm G/D/S");std::swap(f.pins[0],f.pins[1]);break;
    case K::Button:if(p.model=="tactile-12"){f={{1.62,-.92,12,12},{{0,0},{15.24,0},{0,10.16},{15.24,10.16}},"Tactile 12×12 mm · 15.24×10.16 mm formed leads"};break;}f={{.81,-.46,6,6},{{0,0},{7.62,0},{0,5.08},{7.62,5.08}},"Tactile 6×6 mm · formed 7.62×5.08 mm leads"};break;
    case K::Potentiometer:if(p.model.starts_with("slide-")){double travel=p.model=="slide-60"?60:100;f={{0,0,travel+28,14},{{3,7},{travel+25,10.5},{travel+25,3.5}},"B10K slide · solder pins"};}else if(p.model=="rotary-b10k"){row({0,0,16,16},5,20,"B10K rotary · solder pins");}else row({0,0,9.5,9.5},2.54,12.7,"3296W · 2.54 mm");break;
    case K::SlideSwitch:row({0,0,8.5,3.5},2.54,7.62,"Slide SPDT · formed 2.54 mm leads");break;
    case K::ToggleSwitch:row({0,0,12,8},5.08,12.7,"Toggle · solder lugs");break;
    case K::PiezoBuzzer:row({0,0,12,12},7.62,15.24,"Piezo 12 mm · 7.62 mm");break;
    case K::Speaker:row({0,0,15,11},10.16,15.24,"Speaker 15×11 mm · solder pads");break;
    case K::RotaryEncoder:row({0,0,15,15},2.54,20.32,"Encoder contact adapter · 2.54 mm");break;
    case K::SevenSegment:{f={{0,0,12.7,19},{{8.89,1.88},{11.43,1.88},{8.89,17.12},{3.81,17.12},{1.27,17.12},{3.81,1.88},{1.27,1.88},{11.43,17.12},{6.35,17.12},{6.35,1.88}},"Common-cathode single digit · 2.54×15.24 mm"};break;}
    case K::OpAmp:dip(8,"Single op-amp · DIP-8");f.pins={{0,6.58},{0,4.04},{7.62,4.04},{0,9.12},{7.62,6.58}};break;
    case K::AndGate:case K::NandGate:case K::OrGate:case K::NorGate:case K::XorGate:case K::XnorGate:case K::NotGate:dip(8,"Single gate adapter · DIP-8");f.pins=p.kind==K::NotGate?std::vector<QPointF>{{0,4.04},{7.62,6.58},{7.62,1.5},{0,9.12}}:std::vector<QPointF>{{0,4.04},{0,6.58},{7.62,6.58},{7.62,1.5},{0,9.12}};break;
    default:return {};
    }
    return f;
}
inline QRectF bounds(const Package& f){auto r=f.body;for(auto point:f.pins)r=r.united(QRectF(point-QPointF(.3,.3),QSizeF(.6,.6)));return QRectF(r.x()*6,r.y()*6,r.width()*6,r.height()*6).adjusted(-7,-7,7,7);}
inline void text(QPainter& p,QRectF rect,QString text,QColor color,double size){
    // Draw text in scene units so font hinting never stretches glyphs.
    p.save();p.scale(1./6,1./6);rect=QRectF(rect.x()*6,rect.y()*6,rect.width()*6,rect.height()*6);QFont font("Sans Serif");font.setPixelSize(std::max(3,int(size*6)));QFontMetricsF metrics(font);double fit=std::min(1.,rect.width()/std::max(1.,metrics.horizontalAdvance(text)));font.setPixelSize(std::max(2,int(size*6*fit)));p.setFont(font);p.setPen(color);p.setClipRect(rect);p.drawText(rect,Qt::AlignCenter,text);p.restore();
}
inline bool paint(QPainter& p,const CircuitPart& part,const CircuitState& state,const Frame& frame,double brightness,bool selected){
    auto spec=package(part);if(!spec)return false;const auto& f=*spec;using K=PartKind;p.save();p.scale(6,6);p.setRenderHint(QPainter::Antialiasing);
    const QColor metal("#a5afb5"),ink("#263238");
    auto line=[&](QPointF a,QPointF b,double width=.48){p.setPen(QPen(metal,width,Qt::SolidLine,Qt::RoundCap));p.drawLine(a,b);};
    auto box=[&](QRectF r,QColor color,double radius=.35){p.setPen(QPen(color.darker(135),.14));p.setBrush(color);p.drawRoundedRect(r,radius,radius);};
    auto circle=[&](QRectF r,QColor color){p.setPen(QPen(color.darker(135),.14));p.setBrush(color);p.drawEllipse(r);};
    auto current=[&](std::string suffix=""){auto it=state.ledCurrent.find(part.id+suffix);return it==state.ledCurrent.end()?0.:it->second;};
    for(std::size_t index=0;index<f.pins.size();++index){if(part.kind==K::Led&&index==0)continue;const auto pin=f.pins[index];QPointF edge(std::clamp(pin.x(),f.body.left()+.2,f.body.right()-.2),std::clamp(pin.y(),f.body.top()+.2,f.body.bottom()-.2));line(edge,pin);}
    if(part.kind==K::OpAmp||(part.kind>=K::AndGate&&part.kind<=K::NotGate))for(int row=0;row<4;++row){line({0,1.5+row*2.54},{1.2,1.5+row*2.54});line({6.4,1.5+row*2.54},{7.62,1.5+row*2.54});}
    switch(part.kind){
    case K::Ground:p.setPen(QPen(QColor("#617780"),.5,Qt::SolidLine,Qt::RoundCap));p.drawLine(QPointF(0,0),QPointF(0,3));for(int row=0;row<3;++row)p.drawLine(QPointF(-4+row,3+row*1.5),QPointF(4-row,3+row*1.5));break;
    case K::PowerSupply:case K::AnalogSource:case K::Voltmeter:case K::FunctionGenerator:case K::Oscilloscope:case K::LogicAnalyzer:case K::CurrentProbe:{
        const bool meter=part.kind==K::Voltmeter,source=part.kind==K::PowerSupply||part.kind==K::AnalogSource;const auto r=f.body;box(r,meter?QColor("#c9a34e"):QColor("#c0cbcf"),meter?3:1.5);QRectF screen=meter?QRectF(7,15,61,25):source?QRectF(7,12,52,21):QRectF(3,8,39,23);box(screen.adjusted(-1,-1,1,1),ink,.5);p.fillRect(screen,meter?QColor("#b0c2a6"):QColor("#1c333d"));QString value;const auto names=CircuitDocument::pins(part);if(meter&&part.model=="multimeter"){int mode=int(parameter(part,"mode",0));QString unit=mode==1?" A":mode==2?" Ω":" V";value=state.partValues.contains(part.id)?QString::number(state.partValues.at(part.id),'f',mode==2?1:3)+unit:"OL";}else if(meter){auto a=state.voltage.find({part.id,"+"}),b=state.voltage.find({part.id,"-"});value=a!=state.voltage.end()&&b!=state.voltage.end()&&a->second&&b->second?QString::number(*a->second-*b->second,'f',3)+" V":"— V";}else if(source)value=QString::number(part.value,'f',2)+" V";else value=part.kind==K::FunctionGenerator?QString::number(part.value)+" Hz":part.kind==K::Oscilloscope?"CH A / CH B":part.kind==K::LogicAnalyzer?"D0 … D7":"I  + → −";text(p,screen,value,meter?ink:QColor("#7dd7b2"),source?5:meter?5:3.5);
        text(p,{3,1,r.width()-6,meter?10.:5.},QString::fromStdString(componentSpec(part.kind).label),ink,meter?2.:source?2.4:1.3);
        if(source||meter){const auto center=meter?QPointF(37.5,85):QPointF(74,23);circle({center.x()-8,center.y()-8,16,16},QColor("#455b64"));p.setPen(QPen(QColor("#d4dcde"),.7));p.drawLine(center,center+QPointF(2,-6));}
        for(std::size_t i=0;i<f.pins.size();++i){const auto pin=f.pins[i];if(source||meter){circle({pin.x()-3,pin.y()-3,6,6},i?ink:QColor("#a85b55"));circle({pin.x()-1,pin.y()-1,2,2},QColor("#101c21"));}text(p,{pin.x()-2.2,pin.y()-6,4.4,2.2},QString::fromStdString(names[i]),ink,1.2);}break;}
    case K::BatteryAA:box(f.body,ink,1);box(f.body.adjusted(1,2,-1,-2),QColor("#408c94"),.5);box({4.5,-.8,5.5,1.5},metal,.3);text(p,{2,5,10.5,8},"+",Qt::white,4);text(p,{2,20,10.5,12},"AA",Qt::white,3);text(p,{2,35,10.5,5},"1.5 V",Qt::white,2);break;
    case K::Battery9V:box(f.body,ink,1.5);box({0,0,26.5,13},QColor("#b7a177"),1);for(auto pt:f.pins)circle({pt.x()-2.3,pt.y()-2.3,4.6,4.6},metal);text(p,{3,19,20.5,18},"9 V",Qt::white,5);break;
    case K::Battery3V:circle(f.body,metal);circle(f.body.adjusted(1,1,-1,-1),QColor("#d3d7d7"));text(p,{3,3,14,7},"+",ink,4);text(p,{2,10,16,6},"CR2032",ink,2.2);break;
    case K::InfraredRemote:box(f.body,ink,4);circle({6,7,8,8},part.pressed?QColor("#eb9565"):QColor("#c65b5b"));for(int row=0;row<5;++row)for(int col=0;col<3;++col)box({5.+col*12,27.+row*17,10,12},QColor("#c6cfd1"),2);text(p,{5,118,35,12},"NEC · TV",Qt::white,4);break;
    case K::NfcTestCard:box(f.body,part.pressed?QColor("#376c8c"):QColor("#708290"),3);box({8,14,12,10},QColor("#c6af79"),1.5);text(p,{23,12,55,12},"NFC TEST",Qt::white,4);text(p,{8,34,70,8},"SYNTHETIC TAG",Qt::white,2.8);break;
    case K::Keypad:{box(f.body,QColor("#344d5a"),3);const QStringList names{"1","2","3","A","4","5","6","B","7","8","9","C","*","0","#","D"};auto mask=uint32_t(parameter(part,"keys",0));for(int k=0;k<16;++k){QRectF key(4+(k%4)*16,5+(k/4)*18,14,15);box(key,mask&(1u<<k)?QColor("#48b6aa"):QColor(k%4==3?"#c66f72":"#dce3e6"),1.5);text(p,key,names[k],ink,5);}break;}
    case K::Keyboard:{box(f.body,ink,2);auto black=[](int n){n%=12;return n==1||n==3||n==6||n==8||n==10;};auto mask=uint32_t(parameter(part,"keys",0));int white=0;for(int k=0;k<25;++k)if(!black(k)){box({6.+white++*12.5,14,12,50},mask&(1u<<k)?QColor("#43baae"):QColor("#edf0ef"),.4);}white=0;for(int k=0;k<25;++k){if(!black(k)){++white;continue;}box({6+white*12.5-3.5,14,7,31},mask&(1u<<k)?QColor("#43baae"):QColor("#172329"),.3);}text(p,{6,2,188,8},"25 KEY · MATRIX / MIDI",Qt::white,2.8);break;}
    case K::ServoMotor:case K::DcMotor:{const bool servo=part.kind==K::ServoMotor;box(f.body,servo?QColor("#346a97"):metal,servo?1:4);auto center=f.body.center();circle({center.x()-3,center.y()-3,6,6},QColor("#d5dcdf"));double value=state.partValues.contains(part.id)?state.partValues.at(part.id):0;if(servo){p.save();p.translate(center);p.rotate(value-90);p.setPen(QPen(QColor("#e9eceb"),1.4,Qt::SolidLine,Qt::RoundCap));p.drawLine(QPointF(-8,0),QPointF(8,0));p.restore();}break;}
    case K::Oled:{const bool small=CircuitDocument::displayHeight(part)==32;box(f.body,QColor("#1b5a7c"));const double width=small?28.:23.,height=width*CircuitDocument::displayHeight(part)/128.;const QRectF screen((f.body.width()-width)/2,small?1.:5.,width,height);box(screen.adjusted(-.6,-.6,.6,.6),ink,.2);p.fillRect(screen,QColor("#08171e"));auto v=state.voltage.find({part.id,"VCC"}),g=state.voltage.find({part.id,"GND"});if(v!=state.voltage.end()&&g!=state.voltage.end()&&v->second&&g->second&&*v->second-*g->second>=3){QColor light("#91e0ef");light.setAlphaF(brightness);for(int y=0;y<frame.height;++y)for(int x=0;x<frame.width;++x)if(frame.pixel(x,y))p.fillRect(QRectF(screen.left()+x*width/128,screen.top()+y*height/CircuitDocument::displayHeight(part),width/128*.9,height/CircuitDocument::displayHeight(part)*.9),light);}if(!small)text(p,{3,19,21,3},"SSD1306",Qt::white,1.5);for(std::size_t i=0;i<f.pins.size();++i)text(p,{f.pins[i].x()-1.24,f.pins[i].y()-2.8,2.48,1.8},QString::fromStdString(CircuitDocument::pins(part)[i]),Qt::white,.72);break;}
    case K::CharacterLcd:case K::I2cCharacterLcd:{box(f.body,QColor("#277655"),1);const bool large=part.model=="lcd-20x4";const QRectF glass(large?10.:8.,large?14.:10.,large?78.:64.,large?30.:16.);box(glass.adjusted(-2,-2,2,2),QColor("#24352e"),.5);p.fillRect(glass,brightness>.5?QColor("#b5c979"):QColor("#82916b"));int cols=large?20:16,rows=large?4:2;const double pitch=std::min((glass.width()-4)/(cols*6),(glass.height()-2)/(rows*10));const QPointF start=glass.center()-QPointF(cols*6*pitch/2,rows*10*pitch/2);for(int y=0;y<rows*10;++y)for(int x=0;x<cols*6;++x)if(frame.pixel(x,y))p.fillRect(QRectF(start.x()+x*pitch,start.y()+y*pitch,pitch*.85,pitch*.85),QColor("#29412b"));for(auto pt:{QPointF(2.5,2.5),QPointF(f.body.right()-2.5,2.5),QPointF(2.5,f.body.bottom()-2.5),QPointF(f.body.right()-2.5,f.body.bottom()-2.5)})circle({pt.x()-1.25,pt.y()-1.25,2.5,2.5},QColor("#c8b581"));const auto names=CircuitDocument::pins(part);for(std::size_t i=0;i<names.size();++i)text(p,{f.pins[i].x()-1.2,4.3,2.4,2.5},QString::fromStdString(names[i]),Qt::white,.8);text(p,{8,f.body.bottom()-5,f.body.width()-16,3},large?"HD44780 · 20 × 4":"HD44780 · 16 × 2",Qt::white,1.6);break;}
    case K::InfraredReceiver:box(f.body,ink,.6);circle({1.2,.8,4.4,4.4},QColor("#3c4a4f"));break;
    case K::NfcReader:case K::RadioTransceiver:case K::RadioTransmitter:case K::RadioReceiver:{box(f.body,part.kind==K::NfcReader?QColor("#9d3f45"):QColor("#2d6a55"));if(part.kind==K::NfcReader){p.setPen(QPen(QColor("#cbb77b"),.35));p.setBrush(Qt::NoBrush);for(int i=0;i<4;++i)p.drawRoundedRect(f.body.adjusted(2+i,2+i,-2-i,-5-i),1,1);}else{p.setPen(QPen(QColor("#cbb77b"),.55));QPainterPath antenna;antenna.moveTo(2,3);for(int i=0;i<5;++i){antenna.lineTo(2+i*2.8,3);antenna.lineTo(2+i*2.8,8);antenna.lineTo(3.4+i*2.8,8);antenna.lineTo(3.4+i*2.8,3);}p.setBrush(Qt::NoBrush);p.drawPath(antenna);}box({f.body.center().x()-3,f.body.center().y()-3,6,6},ink,.2);text(p,f.body.adjusted(1,10,-1,-3),part.kind==K::NfcReader?"PN532":part.kind==K::RadioTransceiver?"nRF24L01":"433 MHz",Qt::white,1.1);const auto names=CircuitDocument::pins(part);for(std::size_t i=0;i<names.size();++i){circle({f.pins[i].x()-.6,f.pins[i].y()-.6,1.2,1.2},QColor("#bba76c"));if(part.kind!=K::RadioTransceiver)text(p,{f.pins[i].x()-1.2,f.pins[i].y()-2.5,2.4,1.5},QString::fromStdString(names[i]),Qt::white,.7);}break;}
    case K::Resistor:{box(f.body,QColor("#d1ba90"),.8);const std::array<QColor,10> colors{QColor("#202328"),QColor("#704a32"),QColor("#c94741"),QColor("#dc8b3a"),QColor("#decb54"),QColor("#45825a"),QColor("#4378a8"),QColor("#86639b"),QColor("#929899"),QColor("#efece0")};int exponent=int(std::floor(std::log10(std::max(1.,part.value))))-1;int digits=std::clamp(int(std::round(part.value/std::pow(10.,exponent))),10,99);std::array<QColor,4> bands{colors[digits/10],colors[digits%10],exponent<0?QColor("#bda258"):colors[std::clamp(exponent,0,9)],QColor("#bda258")};for(int i=0;i<4;++i){p.fillRect(QRectF(4.0+i*1.3,-1.05,.48,2.1),bands[i]);}break;}
    case K::Diode:case K::TiltSwitch:case K::ReedSwitch:box(f.body,part.kind==K::Diode?QColor("#333c40"):QColor("#94b3ac"),.6);if(part.kind==K::Diode)p.fillRect(QRectF(f.body.right()-1.1,f.body.top()+.1,.6,f.body.height()-.2),QColor("#cad2d5"));else{p.setPen(QPen(ink,.3));p.drawLine(f.body.left()+.4,0,f.body.center().x(),0);p.drawLine(f.body.center(),QPointF(f.body.right()-.4,part.pressed?0:-.6));}break;
    case K::Led:case K::InfraredLed:case K::RGBLed:{QColor color(QString::fromStdString(part.color));if(part.kind==K::InfraredLed)color=QColor("#7056a1");if(part.kind==K::RGBLed){auto c=[&](const char* s){return std::clamp(current(s)/.02,0.,1.);};color=QColor::fromRgbF(c(":R"),c(":G"),c(":B"));if(color==QColor(Qt::black))color=QColor("#b4c3c7");}if(current()>1e-5)color=color.lighter(125);const auto r=f.body;QPainterPath dome;dome.moveTo(r.left(),r.bottom()-1);dome.lineTo(r.left(),r.top()+2.5);dome.cubicTo(r.left(),r.top()-.8,r.right(),r.top()-.8,r.right(),r.top()+2.5);dome.lineTo(r.right(),r.bottom()-1);dome.closeSubpath();p.setPen(QPen(color.darker(150),.15));p.setBrush(color);p.drawPath(dome);box({r.left()-.2,r.bottom()-.6,r.width()+.4,.6},color);p.setPen(QPen(color.darker(170),.2));p.drawLine(QPointF(r.right()-.25,r.bottom()-2),QPointF(r.right()-.25,r.bottom()));if(part.kind==K::Led){auto a=f.pins[0];line({a.x(),r.bottom()},{a.x(),r.bottom()+.5});line({a.x(),r.bottom()+.5},{a.x()+.5,r.bottom()+1.1});line({a.x()+.5,r.bottom()+1.1},{a.x(),r.bottom()+1.7});line({a.x(),r.bottom()+1.7},a);}break;}
    case K::Button:box(f.body,QColor("#b3babd"));box(f.body.adjusted(.45,.45,-.45,-.45),QColor("#d9dddf"));circle(QRectF(f.body.center()-QPointF(f.body.width()*.3,f.body.width()*.3),QSizeF(f.body.width()*.6,f.body.width()*.6)),part.pressed?QColor(QString::fromStdString(part.color)).darker(130):QColor(QString::fromStdString(part.color)));for(auto pt:{f.body.topLeft()+QPointF(.5,.5),f.body.bottomRight()-QPointF(.5,.5)})circle({pt.x()-.2,pt.y()-.2,.4,.4},ink);break;
    case K::Photoresistor:case K::Thermistor:circle(f.body,part.kind==K::Photoresistor?QColor("#c7b18b"):QColor("#47647c"));if(part.kind==K::Photoresistor){p.setPen(QPen(QColor("#845741"),.3));QPainterPath trace;trace.moveTo(1,1);for(int i=0;i<5;++i){trace.lineTo(i%2?1:4,1+i*.65);trace.lineTo(i%2?1:4,1.5+i*.65);}p.setBrush(Qt::NoBrush);p.drawPath(trace);}break;
    case K::Capacitor:circle(f.body.adjusted(0,0,0,-4.7),QColor("#2b7690"));box(f.body,QColor("#2b7690"),.8);circle({0,0,6.3,1.7},QColor("#c2c9ca"));p.setPen(QPen(ink,.15));p.drawLine(QPointF(1.3,.85),QPointF(5,.85));p.fillRect(QRectF(part.model=="polarized"?5.1:.5,2,.65,8.2),QColor("#acb8b9"));text(p,{1.4,3,4.4,4},QString::number(part.value)+"µF",Qt::white,1.2);break;
    case K::Inductor:circle(f.body,QColor("#414b50"));p.setPen(QPen(QColor("#b97d4f"),.55));for(int i=0;i<7;++i)p.drawEllipse(QRectF(.5+i*.85,1.2,1.8,5.6));break;
    case K::NpnTransistor:case K::PnpTransistor:case K::TemperatureSensor:box(f.body,ink,1.1);text(p,f.body,part.kind==K::TemperatureSensor?"TMP36":part.kind==K::NpnTransistor?"NPN":"PNP",Qt::white,1.05);break;
    case K::NMosfet:case K::PMosfet:box(f.body,ink,.2);box({0,0,10.16,4.5},metal,.15);circle({3.58,.5,3,3},QColor("#ecedef"));text(p,{.7,6,8.7,5},part.kind==K::NMosfet?"N-MOS":"P-MOS",Qt::white,1.35);break;
    case K::Potentiometer:if(part.model.starts_with("slide-")){box(f.body,QColor("#c0c6c7"));box(f.body.adjusted(4,5,-4,-5),ink,.4);double x=14+part.value/100*(f.body.width()-28);box({x-3,1,6,12},QColor("#434d52"),.7);p.setPen(QPen(metal,.3));p.drawLine(QPointF(x,3),QPointF(x,11));text(p,{6,0,f.body.width()-12,3},"B10K",ink,1.1);break;}if(part.model=="rotary-b10k"){circle(f.body,QColor("#baa66e"));circle(f.body.adjusted(4,4,-4,-4),QColor("#9aa5aa"));p.setPen(QPen(ink,.4));for(int i=0;i<16;++i){double angle=i*6.2831853/16;auto c=f.body.center();p.drawLine(c+QPointF(std::sin(angle)*3,std::cos(angle)*3),c+QPointF(std::sin(angle)*4,std::cos(angle)*4));}break;}box(f.body,QColor("#28668a"));circle({5.4,.8,3.2,3.2},QColor("#b6a478"));p.save();p.translate(7,2.4);p.rotate(part.value*2.7-135);p.setPen(QPen(ink,.3));p.drawLine(QPointF(-1,0),QPointF(1,0));p.restore();text(p,{.6,5,8.3,2.5},"3296 103",Qt::white,1.1);break;
    case K::SlideSwitch:case K::ToggleSwitch:box(f.body,metal);box(f.body.adjusted(1,.5,-1,-.5),ink);box({part.pressed?f.body.width()-3.2:1.2,.3,2, f.body.height()-.6},QColor("#4d5b60"));break;
    case K::PiezoBuzzer:circle(f.body,ink);circle({5,5,2,2},QColor("#12191c"));text(p,{1,7,3,3},"+",Qt::white,1.8);break;
    case K::Speaker:box(f.body,ink,1.1);circle({3,1,9,9},QColor("#637074"));circle({5.5,3.5,4,4},ink);break;
    case K::RotaryEncoder:box(f.body,metal);circle({2,2,11,11},ink);p.save();p.translate(7.5,7.5);p.rotate(parameter(part,"phase",0)*90);p.setPen(QPen(QColor("#ccd5d7"),.5));p.drawLine(QPointF(0,-4),QPointF(0,-1));p.restore();break;
    case K::SevenSegment:{box(f.body,ink);const std::array<QLineF,7> segments{QLineF(3,2.5,9,2.5),QLineF(9.5,3,9.5,8),QLineF(9.5,10,9.5,15),QLineF(3,15.5,9,15.5),QLineF(2.5,10,2.5,15),QLineF(2.5,3,2.5,8),QLineF(3,9,9,9)};for(int i=0;i<7;++i){p.setPen(QPen(current(":"+std::string(1,char('a'+i)))>1e-5?QColor("#fa5a59"):QColor("#493337"),.8,Qt::SolidLine,Qt::RoundCap));p.drawLine(segments[i]);}circle({10.4,15,.8,.8},current(":dp")>1e-5?QColor("#fa5a59"):QColor("#493337"));break;}
    default:box(f.body,ink);circle({3.05,.3,1.5,1.5},QColor("#526168"));text(p,f.body.adjusted(.6,2,-.6,-.6),part.kind==K::OpAmp?"OP AMP":QString::fromStdString(componentSpec(part.kind).label),Qt::white,1.2);break;
    }
    if(selected){p.setPen(QPen(QColor("#14b8a6"),.2,Qt::DashLine));p.setBrush(Qt::NoBrush);p.drawRoundedRect(f.body.adjusted(-.7,-.7,.7,.7),.6,.6);}p.restore();return true;
}
}
