#pragma once
#include "thinkercad/modules.hpp"
#include <QPainter>
#include <QPainterPath>
#include <cmath>

// Flat top views in physical millimetres. No stretched bitmaps or generic
// labelled rectangles: each family shows its actual mechanical construction.
namespace ModuleIllustrations {
using namespace thinkercad;
struct Drawing {
    QPainter& p;const CircuitPart& part;const ModuleDefinition& d;
    QColor ink{"#242c30"},metal{"#bdc5c7"},gold{"#c8a75a"};
    void box(QRectF r,QColor c,double radius=.3){p.setPen(QPen(c.darker(125),.12));p.setBrush(c);p.drawRoundedRect(r,radius,radius);}
    void disc(QPointF at,double radius,QColor c){p.setPen(Qt::NoPen);p.setBrush(c);p.drawEllipse(at,radius,radius);}
    void line(QPointF a,QPointF b,QColor c,double width=.25){p.setPen(QPen(c,width,Qt::SolidLine,Qt::RoundCap));p.drawLine(a,b);}
    void label(QRectF r,QString s,double size=1,QColor color=Qt::white){p.setPen(color);QFont f("Sans Serif");f.setPointSizeF(size);p.setFont(f);p.drawText(r,Qt::AlignCenter,s);}
    void hole(QPointF at){disc(at,1.4,gold);disc(at,.9,QColor("#edf0eb"));}
    void board(QColor color=QColor("#256a7a"),bool mounts=true){box({0,0,d.width,d.height},color,.6);if(mounts&&d.width>12&&d.height>12)for(auto at:{QPointF(2,2),QPointF(d.width-2,2),QPointF(2,d.height-2),QPointF(d.width-2,d.height-2)})hole(at);}
    void smd(QPointF at,bool capacitor=false){box({at.x()-.8,at.y()-.4,1.6,.8},metal,.05);box({at.x()-.48,at.y()-.4,.96,.8},capacitor?QColor("#c1a67e"):ink,.03);}
    void chip(QRectF r,QString name,int pins=8){for(int i=0;i<pins/2;++i){double y=r.top()+(i+.5)*r.height()/(pins/2);line({r.left()-.6,y},{r.right()+.6,y},metal,.3);}box(r,ink,.15);disc(r.topLeft()+QPointF(.6,.6),.2,QColor("#798286"));label(r,name,std::min(1.1,r.width()/std::max<qsizetype>(1,name.size())*1.2),QColor("#c8d0cd"));}
    void pins(){
        for(std::size_t i=0;i<d.pins.size();++i){const auto& name=d.pins[i];QPointF at;auto found=d.pinPositions.find(name);if(found!=d.pinPositions.end())at={found->second.first,found->second.second};else{int half=int((d.pins.size()+1)/2),row=i>=std::size_t(half)?int(i)-half:int(i),count=i>=std::size_t(half)?int(d.pins.size())-half:half;at={i>=std::size_t(half)?d.width:0,d.height*(row+1)/(count+1)};}
            box({at.x()-.75,at.y()-.75,1.5,1.5},ink,.08);box({at.x()-.34,at.y()-.34,.68,.68},gold,.02);
            if(d.pins.size()<20&&!name.starts_with("NC")){if(at.y()>d.height-1)label({at.x()-1.2,at.y()-3,2.4,1.7},QString::fromStdString(name),.65);else label({at.x()<d.width/2?at.x()+1:at.x()-7,at.y()-.8,6,1.6},QString::fromStdString(name),.75);}
        }
    }
    void knob(QPointF at,double radius,const std::string& field,double low,double high,double fallback){
        double value=parameter(part,field,fallback),angle=(-135+std::clamp((value-low)/(high-low),0.,1.)*270)*3.141592653589793/180;
        disc(at,radius+.4,metal);disc(at,radius,ink);line(at+QPointF(std::sin(angle),-std::cos(angle))*(radius*.4),at+QPointF(std::sin(angle),-std::cos(angle))*(radius*.85),QColor("#e4e9e5"),.35);
    }
};
inline bool paint(QPainter& painter,const CircuitPart& part,const CircuitState& state){
    const auto& d=ModuleRegistry::get(part.model);const auto& id=part.model;Drawing a{painter,part,d};auto& p=painter;const double w=d.width,h=d.height;const QColor pcb("#286a54"),plastic("#252d31"),metal("#b9c2c3");
    if(id=="ili9341"||id=="st7735"){
        a.board(QColor("#27518b"));const double screenWidth=w-6,screenHeight=screenWidth*(id=="ili9341"?320./240:160./128);
        const QRectF screen(3,4,screenWidth,screenHeight);a.box(screen,QColor("#080b0f"),.4);
        if(auto found=state.colorDisplays.find(part.id);found!=state.colorDisplays.end()&&found->second){const auto& frame=*found->second;
            QImage image(reinterpret_cast<const uchar*>(frame.pixels.data()),frame.width,frame.height,frame.width*4,QImage::Format_ARGB32);
            p.save();p.setOpacity(frame.brightness);p.drawImage(screen,image);p.restore();}
        a.label({2,h-5,w-4,4},id=="ili9341"?"ILI9341":"ST7735",1.5);a.pins();return true;
    }
    if(id.starts_with("stepper-")){
        const QPointF center(w/2,h/2);a.box({0,0,w,h},QColor("#5d656b"),2);
        for(double x:{3.,w-3})for(double y:{3.,h-3})a.hole({x,y});
        a.disc(center,std::min(w,h)*.36,QColor("#232a30"));a.disc(center,std::min(w,h)*.27,metal);
        const auto found=state.partValues.find(part.id);const double angle=found==state.partValues.end()?0:std::fmod(found->second,360.);
        p.save();p.translate(center);p.rotate(angle);a.box({-2,-2,4,4},QColor("#d2d6d7"));a.line({0,0},{0,-std::min(w,h)*.23},QColor("#222c31"),.8);p.restore();
        a.label({3,h-8,w-6,4},QString("%1°").arg(angle,0,'f',1),1.5);a.pins();return true;
    }
    if(id=="limit-switch"){
        const bool pressed=parameter(part,"pressed",0)>0;a.box({0,2,w,h-2},QColor("#30363a"),1);a.line({2,3},{w-2,pressed?2.:1.3},metal,.7);a.disc({w-2,pressed?2.:1.3},1.2,metal);a.label({1,4,w-2,3},pressed?"COM → NO":"COM → NC",1);a.pins();return true;
    }
    if(id=="relay-5v"){
        a.board(pcb);a.box({3,3,w-6,h*.48},QColor("#3076a8"),.8);a.label({3,6,w-6,6},"5 V RELAY",1.5);
        const bool on=state.partValues.contains(part.id)&&state.partValues.at(part.id)>0;
        a.disc({w-5,h*.65},1,on?QColor("#52de86"):QColor("#34583c"));
        a.line({w*.3,h*.8},{w*(on?.7:.45),h*.68},metal,.6);
        a.disc({w*.45,h*.68},.7,metal);a.disc({w*.7,h*.68},.7,metal);
        a.label({2,h-6,w-4,3},on?"COM → NO":"COM → NC",1.2);a.pins();return true;
    }
    if(id=="joystick"){
        a.board(pcb);a.box({4,5,18,18},metal,1.2);a.box({2,8,4,12},QColor("#43866c"),.5);a.box({7,2.6,12,4},QColor("#43866c"),.5);
        for(double y:{7.,20.}){a.line({5,y},{21,y},QColor("#79888b"),.35);a.box({4,y-.6,2,1.2},metal);a.box({20,y-.6,2,1.2},metal);}
        a.disc({13,14},10,QColor("#767f82"));a.disc({13,14},8.7,plastic);
        QPointF cap(13+(parameter(part,"x",50)-50)/50*5,14-(parameter(part,"y",50)-50)/50*5);
        a.disc(cap,7,QColor("#20282c"));p.setPen(QPen(QColor("#566065"),.3));p.setBrush(Qt::NoBrush);p.drawEllipse(cap,6,6);a.disc(cap,4.6,QColor("#30393d"));a.disc(cap,1.1,QColor("#262f33"));
        a.box({18,27,6,4},parameter(part,"pressed",0)?QColor("#47a994"):plastic,.6);a.label({18,27,6,4},"SW",1.2);a.pins();return true;
    }
    if(id=="arcade-button"){
        QPointF center(w/2,h/2);a.disc(center,15,QColor("#1c2529"));a.disc(center,13.7,QColor("#798486"));QColor color(QString::fromStdString(part.color));if(!color.isValid())color=QColor("#e94d55");bool down=parameter(part,"pressed",0);a.disc(center,down?11.8:12.5,down?color.darker(125):color);p.setPen(QPen(color.lighter(125),.4));p.setBrush(Qt::NoBrush);p.drawEllipse(center,10.8,10.8);return true;
    }
    if(id=="drum-pad"){
        a.box({0,0,w,h},plastic,2.5);bool down=parameter(part,"strike",0)>0;a.box({2,2,w-4,h-4},down?QColor("#707c7b"):QColor("#505e60"),2);p.setPen(QPen(QColor("#808e8d"),.2));p.setBrush(Qt::NoBrush);p.drawRoundedRect(QRectF(4,4,w-8,h-8),1.2,1.2);return true;
    }
    if(id=="softpot"){
        a.box({0,0,w,h},QColor("#b89451"),.6);a.box({.7,1,w-1.4,h-7},QColor("#e4e0c8"),.4);a.box({3,7,6,h-15},QColor("#687b79"),1.5);a.line({6,9},{6,h-10},QColor("#b7c1ac"),.2);
        if(parameter(part,"touch",0)){double y=8+parameter(part,"position",50)/100*84;a.box({2.4,y-1.6,7.2,3.2},QColor("#3dbba7"),1);}
        a.pins();return true;
    }
    if(id=="fsr"){
        a.box({5,15,8,h-15},QColor("#cdc59a"),.3);for(double x:{7.73,10.27})a.line({x,16},{x,h},QColor("#a7a083"),.5);
        a.disc({9,9},9,QColor("#354643"));a.disc({9,9},7.5,QColor("#7a8e7d"));p.setPen(QPen(QColor("#bfc9af"),.3));p.setBrush(Qt::NoBrush);p.drawEllipse(QPointF(9,9),6.4,6.4);
        for(int n=0;n<8;++n){double y=4.1+n*1.4;a.line({5,y},{13,y},QColor("#a5b49a"),.3);}if(parameter(part,"level",0)>0)a.disc({9,9},2,QColor("#42b49e"));a.pins();return true;
    }
    if(id=="photodiode"||id=="ds18b20"||id=="tl431"){
        for(const auto& [name,at]:d.pinPositions)a.line({at.first,4},{at.first,at.second},metal,.4);
        if(id=="photodiode"){a.disc({w/2,2.5},2.5,QColor("#323c48"));a.disc({w/2,2.5},1.6,QColor("#526779"));}
        else{QPainterPath shape;shape.moveTo(.1,5.5);shape.lineTo(.1,2);shape.cubicTo(.1,-.4,w-.1,-.4,w-.1,2);shape.lineTo(w-.1,5.5);shape.closeSubpath();p.setPen(QPen(QColor("#151f24"),.15));p.setBrush(plastic);p.drawPath(shape);a.label({.3,2,w-.6,3},id=="ds18b20"?"18B20":"431",.75);}
        return true;
    }
    if(id.starts_with("toggle-")){
        a.box({0,0,w,h},QColor("#6f8797"),.6);a.disc({w/2,h/2},3.2,metal);p.setPen(QPen(QColor("#69767b"),.3));p.setBrush(Qt::NoBrush);p.drawEllipse(QPointF(w/2,h/2),2.5,2.5);
        int pos=int(parameter(part,"position",0));QPointF tip(w/2,h/2+(pos==1?-3.7:pos==2?3.7:0));a.line({w/2,h/2},tip,plastic,1.8);a.disc(tip,1.3,QColor("#e4e7e0"));a.pins();return true;
    }
    if(id=="active-buzzer"){
        a.disc({6,6},6,plastic);p.setPen(QPen(QColor("#59676b"),.2));p.setBrush(Qt::NoBrush);p.drawEllipse(QPointF(6,6),5.1,5.1);a.disc({6,6},1.8,QColor("#111a1e"));a.label({1.3,.7,3,3},"+",1.8);return true;
    }
    if(id=="soil-moisture"){
        QPainterPath outline;outline.moveTo(0,0);outline.lineTo(w,0);outline.lineTo(w,h-6);outline.lineTo(w-4,h);outline.lineTo(w-6,h-6);outline.lineTo(w-6,11);outline.lineTo(6,11);outline.lineTo(6,h-6);outline.lineTo(4,h);outline.lineTo(0,h-6);outline.closeSubpath();p.setPen(QPen(QColor("#8f7140"),.15));p.setBrush(QColor("#32594d"));p.drawPath(outline);
        for(double x:{2.,w-4})for(int y=13;y<h-5;y+=3)a.box({x,double(y),2,1.3},a.gold,.05);a.chip({6,2,8,5},"393");a.pins();return true;
    }
    if(id=="ambient-light"){
        a.board();a.disc({w/2,9},6,QColor("#c5b287"));p.setPen(QPen(QColor("#6b4c38"),.65));for(int n=0;n<5;++n){double y=5+n*1.7;p.drawLine(QPointF(7,y),QPointF(13,y));if(n<4)p.drawLine(QPointF(n%2?7:13,y),QPointF(n%2?7:13,y+1.7));}a.chip({6,19,8,5},"LM393");a.box({2,20,3,5},QColor("#2f71ad"));a.pins();return true;
    }
    if(id=="lemon"||id=="potato"){
        QPainterPath outline;outline.moveTo(0,h*.5);outline.cubicTo(w*.12,h*.4,w*.1,h*.1,w*.35,h*.06);outline.cubicTo(w*.65,-h*.06,w*.87,h*.15,w,h*.5);outline.cubicTo(w*.85,h*.7,w*.77,h*.96,w*.5,h*.98);outline.cubicTo(w*.25,h*1.02,w*.07,h*.8,0,h*.5);p.setPen(QPen(QColor(id=="lemon"?"#b99e3f":"#896748"),.2));p.setBrush(QColor(id=="lemon"?"#e6cd60":"#af8b61"));p.drawPath(outline);
        for(int i=0;i<6;++i)a.disc({w*(.22+(i%3)*.24),h*(.3+(i/3)*.35)},id=="lemon"?.25:.8,QColor(id=="lemon"?"#cdb755":"#8f7358"));a.box({w*.27,4,3,12},QColor("#b57e51"),.1);a.box({w*.67,4,3,12},metal,.1);a.label({w*.24,17,7,5},"+",3,plastic);a.label({w*.64,17,7,5},"−",3,plastic);return true;
    }
    if(id.starts_with("header-")){
        a.box({0,0,w,h},plastic,.12);for(const auto& [pin,at]:d.pinPositions){a.box({at.first-.7,at.second-.7,1.4,1.4},QColor("#444d50"),.08);a.box({at.first-.32,at.second-.32,.64,.64},a.gold,.03);}return true;
    }
    if(id.starts_with("terminal-")){
        int count=int(d.pins.size());a.box({0,0,w,h},QColor("#397f6c"),.2);for(int n=0;n<count;++n){double x=(n+.5)*w/count;a.disc({x,2.4},1.15,metal);a.line({x-.7,1.9},{x+.7,2.9},plastic,.3);a.box({x-1,h-3.3,2,2.4},plastic,.1);}return true;
    }
    if(id.starts_with("jst-")){
        a.box({0,0,w,h},QColor("#e4e4d6"),.4);a.box({1,1,w-2,h-2},QColor("#666d67"),.15);for(int n=0;n<int(d.pins.size());++n){double x=(n+.5)*w/d.pins.size();a.box({x-.7,1,1.4,h-2},QColor("#e4e4d6"),.1);a.box({x-.25,2,.5,h-4},a.gold,.05);}a.box({w/2-1,0,2,1},QColor("#f1f0e5"),.15);return true;
    }
    if(id.starts_with("midi-din-")){
        a.disc({w/2,h/2},w/2,plastic);a.disc({w/2,h/2},w*.39,metal);a.disc({w/2,h/2},w*.32,QColor("#303b3e"));for(int n=0;n<5;++n){double angle=(45+n*67.5)*3.141592653589793/180;a.disc({w/2+std::sin(angle)*w*.22,h/2-std::cos(angle)*w*.22},.65,QColor("#10191d"));}a.box({w/2-.5,h*.76,1,1.3},metal,.1);return true;
    }
    if(id.find("jack")!=std::string::npos){
        if(id=="jack-cable-trrs"){a.line({4,h/2},{w-4,h/2},plastic,1.5);for(double x:{1.,w-6}){a.box({x,h/2-1.5,5,3},metal,.5);for(int n=1;n<4;++n)a.box({x+n,h/2-1.5,.3,3},plastic,.05);}}else {a.box({1,0,w-1,h},plastic,.4);a.box({0,h*.2,w*.45,h*.6},metal,.3);a.disc({w*.22,h/2},std::min(w*.18,h*.22),QColor("#10191d"));a.pins();}return true;
    }
    if(id=="rca"||id=="banana"||id=="dc-barrel"||id=="usb-c-power"){
        if(id=="banana"){for(int n=0;n<2;++n){QPointF c((n+.5)*w/2,h/2);a.disc(c,3.6,n?plastic:QColor("#c44343"));a.disc(c,2,metal);a.disc(c,1.25,QColor("#172226"));}}
        else if(id=="rca"){QPointF c(w/2,h/2);a.disc(c,w*.48,metal);a.disc(c,w*.38,QColor("#cc514a"));a.disc(c,1.4,a.gold);a.disc(c,.8,plastic);}
        else if(id=="dc-barrel"){a.box({0,0,w,h},plastic,.5);a.box({0,0,w,h*.35},metal,.4);a.disc({w/2,h*.18},w*.32,QColor("#172226"));a.disc({w/2,h*.18},.65,metal);}
        else {a.board();a.box({w/2-4.4,0,8.8,7.4},metal,.8);a.box({w/2-3.6,.4,7.2,1.7},plastic,.65);a.chip({w/2-1.3,9,2.6,2},"",6);a.pins();}return true;
    }
    if(id=="microbit-breakout"){
        a.board(QColor("#b84346"));a.box({2,2,w-4,7},plastic,.4);a.box({3,4,w-6,1.8},QColor("#131d20"),.2);for(double x=4;x<w-3;x+=1.27)a.box({x,4.3,.5,1},a.gold,.05);a.pins();return true;
    }
    if(id=="r2r-dac"){
        a.board(QColor("#8c7051"),false);for(int bit=0;bit<8;++bit){double x=2+bit*(w-4)/8;a.line({x,2},{x,h-2},a.gold,.2);a.box({x-.6,4,1.2,4},QColor("#dfc194"),.25);for(double y:{4.8,5.5,6.6})a.box({x-.6,y,1.2,.3},QColor("#784f34"),.01);}a.pins();return true;
    }
    if(id=="hcsr04"||id=="ping3"){
        a.board(QColor("#307e9d"));for(double x:{w*.25,w*.75}){a.disc({x,h*.46},h*.4,metal);a.disc({x,h*.46},h*.33,QColor("#4f5d63"));p.setPen(QPen(QColor("#9faeb0"),.15));p.setBrush(Qt::NoBrush);for(int ring=0;ring<4;++ring)p.drawEllipse(QPointF(x,h*.46),h*(.1+ring*.06),h*(.1+ring*.06));}a.pins();return true;
    }
    if(id=="dht22"){
        a.box({0,0,w,h-3},QColor("#e6e9df"),.8);for(int row=0;row<7;++row)for(int col=0;col<3;++col)a.box({2+col*3.7,2+row*2.5,2.5,1.5},QColor("#677774"),.15);a.pins();return true;
    }
    if(id=="l298n"){
        a.board(QColor("#a93f3c"));a.box({11,7,21,23},plastic,.6);for(int n=0;n<8;++n)a.box({12+n*2.5,7,1.3,23},QColor("#4a5355"),.1);for(double x:{1.,w-9}){a.box({x,10,8,17},QColor("#387ca7"));for(int n=0;n<3;++n){a.disc({x+4,13.+n*5},1.5,metal);a.line({x+3,13.+n*5},{x+5,13.+n*5},plastic,.3);}}a.pins();return true;
    }
    if(id=="mfrc522"){
        a.board(QColor("#287999"));p.setPen(QPen(QColor("#b1bbb1"),.35));p.setBrush(Qt::NoBrush);for(int n=0;n<5;++n)p.drawRoundedRect(QRectF(3+n,3+n,w-6-2*n,h*.6-2*n),1,1);a.chip({w*.35,h*.66,w*.3,8},"RC522",16);a.box({4,h*.73,6,3},metal,.5);a.pins();return true;
    }
    if(id=="mpr121"){
        a.board(QColor("#695399"));for(int n=0;n<12;++n){QPointF at((n%6+.5)*w/6,n<6?h*.3:h*.67);a.disc(at,2.3,a.gold);a.disc(at,1.8,(int(parameter(part,"touchMask",0))&(1<<n))?QColor("#6ed2b8"):QColor("#b69854"));a.label({at.x()-2,at.y()-1.2,4,2.4},QString::number(n),.9,plastic);}a.chip({w/2-2,h/2-2,4,4},"121",12);a.pins();return true;
    }
    const bool circuitModule=id=="bme280"||id=="bme280-spi"||id=="mpu6050"||id=="mcp4725"||id=="ad9833"||id=="ad9850"||id=="pcm5102a"||id=="wm8731"||id=="cs4272"||id=="vs1053b"||id=="pt2258"||id=="ssi2144"||id=="pt2399"||id=="fv1"||id=="noise-generator"||id=="pam8403"||id=="tpa3116d2";
    if(!circuitModule)return false;
    a.board(id=="vs1053b"?QColor("#b84246"):id=="mcp4725"?QColor("#72518d"):id=="bme280"||id=="bme280-spi"||id=="mpu6050"?QColor("#307e9d"):pcb);
    if(id=="bme280"||id=="bme280-spi"){
        a.box({w/2-2,h*.3,4,4},metal,.25);a.disc({w/2+1,h*.3+1},.25,plastic);a.smd({w*.25,h*.65});a.smd({w*.7,h*.66},true);
    }else if(id=="mpu6050"){
        a.chip({w/2-2,h/2-2,4,4},"6050",12);a.chip({2,h*.65,3,2},"",6);a.smd({w-4,h*.65},true);a.disc({4,4},.5,QColor("#83b565"));
    }else if(id=="mcp4725"){
        a.chip({w/2-1.5,h*.35,3,2},"",6);for(int i=0;i<3;++i)a.smd({3.+i*3,h*.63},i==2);
    }else if(id=="ad9833"||id=="ad9850"){
        a.chip({w*.2,h*.22,w*.6,h*.22},id=="ad9833"?"9833":"AD9850",id=="ad9850"?28:10);a.box({w*.17,h*.55,w*.66,h*.18},metal,.5);a.label({w*.17,h*.55,w*.66,h*.18},id=="ad9850"?"125 MHz":"25 MHz",1,plastic);for(int n=0;n<3;++n)a.smd({w*.3+n*2,h*.8},true);
    }else if(id=="pam8403"||id=="tpa3116d2"){
        if(id=="tpa3116d2"){a.box({w*.3,h*.2,w*.4,h*.45},plastic);for(int n=0;n<7;++n)a.box({w*.31+n*w*.055,h*.2,w*.025,h*.45},QColor("#606b6e"));for(double x:{w*.17,w*.83}){a.disc({x,h*.4},3,plastic);a.disc({x,h*.4},2.6,metal);a.line({x-1.5,h*.4},{x+1.5,h*.4},QColor("#737f82"),.3);}}
        else a.chip({w*.3,h*.25,w*.4,h*.4},"PAM8403",16);
        for(int n=0;n<4;++n)a.smd({w*(n+1)/5,h*.8},true);
    }else if(id=="ssi2144"||id=="pt2399"||id=="fv1"){
        a.chip({w*.27,h*.2,w*.46,h*.3},id=="ssi2144"?"SSI2144":id=="pt2399"?"PT2399":"FV-1",id=="fv1"?28:16);
        const std::vector<std::string> fields=id=="ssi2144"?std::vector<std::string>{"cutoff","resonance"}:id=="pt2399"?std::vector<std::string>{"delay","feedback","mix"}:std::vector<std::string>{"mix","delay","feedback"};
        for(std::size_t i=0;i<fields.size();++i){const auto& specs=d.parameters;auto f=std::find_if(specs.begin(),specs.end(),[&](const auto& v){return v.name==fields[i];});a.knob({w*(i+.5)/fields.size(),h*.77},std::min(3.,w/fields.size()*.28),f->name,f->min,f->max,f->initial);a.label({w*i/fields.size(),h*.9,w/fields.size(),h*.08},QString::fromStdString(fields[i]),.85);}
    }else if(id=="noise-generator"){
        a.chip({w*.3,h*.2,w*.4,4},"NOISE",8);for(int n=0;n<4;++n){a.smd({4.+n*3,h*.5});a.smd({4.+n*3,h*.65},true);}a.label({2,h*.76,w-4,3},parameter(part,"color",0)==0?"WHITE":parameter(part,"color",0)==1?"PINK":"BROWN",1.1);
    }else {
        a.chip({w*.31,h*.22,w*.38,h*.28},id=="vs1053b"?"VS1053":QString::fromStdString(id).toUpper(),id=="vs1053b"?32:20);
        a.box({w*.18,h*.56,w*.24,3.2},metal,.4);for(int n=0;n<4;++n)a.smd({w*(n+1)/5,h*.71},n%2);
        if(id=="vs1053b"){a.box({2,h-12,9,9},plastic,.7);a.disc({6.5,h-7.5},2,metal);a.disc({6.5,h-7.5},1.3,plastic);a.box({w-16,h-14,13,11},metal,.5);a.box({w-14,h-13,9,2},plastic,.15);}
        if(id=="wm8731"||id=="cs4272")for(double x:{w*.25,w*.75}){a.box({x-4,h-8,8,6},plastic,.4);a.disc({x,h-5},1.5,metal);a.disc({x,h-5},.9,plastic);}
    }
    a.pins();return true;
}
}
