#pragma once
#include "thinkercad/circuit.hpp"
#include <QPainter>
inline bool poweredBoard(const thinkercad::CircuitPart& p){return p.kind==thinkercad::PartKind::Breadboard30&&p.model=="power-rails";}
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
