#pragma once
#include "thinkercad/circuit.hpp"
#include <QPainter>
inline bool poweredBoard(const thinkercad::CircuitPart& p){return p.kind==thinkercad::PartKind::Breadboard30&&thinkercad::breadboardRails(p);}
inline QPointF railBoardPin(const std::string& pin){
    if(pin.size()>2&&(pin[0]=='T'||pin[0]=='B')){const int column=std::stoi(pin.substr(2))-1;return {30+(column+column/5)*15.24,(pin[0]=='T'?15.24:274.32)+(pin[1]=='-'?15.24:0)};}
    const int column=std::stoi(pin.substr(1))-1,row=pin[0]-'a';return {30+column*15.24,78+row*15.24+(row>=5?30.48:0)};
}
inline void paintRailBoard(QPainter& p){
    p.setPen(QPen(QColor("#b8b8b2"),1));p.setBrush(QColor("#eeeae1"));p.drawRoundedRect(QRectF(0,0,503,330),6,6);
    p.setPen(Qt::NoPen);p.setBrush(QColor("#c5c3bc"));p.drawRoundedRect(QRectF(10,154,483,15),3,3);
    p.setFont(QFont("Sans Serif",7));for(int rail=0;rail<4;++rail){const double y=(rail<2?15.24:274.32)+(rail%2?15.24:0);p.setPen(QPen(QColor(rail%2?"#2879b9":"#d74343"),2));p.drawLine(QPointF(14,y+6),QPointF(483,y+6));p.drawText(QRectF(3,y-6,15,14),rail%2?"−":"+");}
    p.setPen(QColor("#73746e"));for(int row=0;row<10;++row)p.drawText(QRectF(3,71+row*15.24+(row>=5?30.48:0),14,14),QString(QChar('a'+row)));for(int col=1;col<=30;++col)if(col==1||col%5==0)p.drawText(QRectF(23+(col-1)*15.24,48,17,18),QString::number(col));
}

inline QRectF breadboardBody(const thinkercad::CircuitPart& part){const auto columns=thinkercad::breadboardColumns(part);return {0,0,poweredBoard(part)?503+(columns-30)*15.24:6*(columns==17?45.7:columns==63?165.1:83.8),poweredBoard(part)?330.:35.6*6};}
inline QPointF breadboardPin(const thinkercad::CircuitPart& part,const std::string& pin){
    if(poweredBoard(part))return railBoardPin(pin);
    const int column=std::stoi(pin.substr(1))-1,row=pin[0]-'a';return {6*(3.5+column*2.54),6*(5.+row*2.54+(row>=5?5.08:0))};
}
inline void paintBreadboard(QPainter& p,const thinkercad::CircuitPart& part){
    const auto body=breadboardBody(part);const bool rails=poweredBoard(part);const auto columns=thinkercad::breadboardColumns(part);
    p.setPen(QPen(QColor("#bfc0bb"),1));p.setBrush(QColor("#e6e7e2"));p.drawRoundedRect(body,4,4);
    const double center=rails?154.:18.2*6;p.setPen(Qt::NoPen);p.setBrush(QColor("#c8c9c4"));p.drawRoundedRect(QRectF(6,center,body.width()-12,15.24),1,1);
    if(rails)for(int rail=0;rail<4;++rail){double y=(rail<2?15.24:274.32)+(rail%2?15.24:0);p.setPen(QPen(QColor(rail%2?"#408ba7":"#c96562"),1.2));p.drawLine(QPointF(12,y+7),QPointF(body.right()-12,y+7));p.drawText(QRectF(2,y-6,12,14),rail%2?"−":"+");}
    QFont font("Sans Serif");font.setPixelSize(6);p.setFont(font);p.setPen(QColor("#757872"));for(int i=1;i<=columns;++i)if(i==1||i%5==0){auto at=breadboardPin(part,"a"+std::to_string(i));p.drawText(QRectF(at.x()-6,rails?48:5,12,12),Qt::AlignCenter,QString::number(i));}
    for(char row='a';row<='j';++row){auto at=breadboardPin(part,std::string(1,row)+"1");p.drawText(QRectF(2,at.y()-5,10,10),Qt::AlignCenter,QString(QChar(row)));}
}
