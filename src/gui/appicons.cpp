#include "appicons.hpp"
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
QIcon AppIcons::icon(Symbol symbol){
    QIcon result;
    for(int size:{24,32,48,64}){
        QPixmap image(size,size);image.fill(Qt::transparent);QPainter p(&image);p.setRenderHint(QPainter::Antialiasing);p.scale(size/32.0,size/32.0);
        p.setPen(Qt::NoPen);
        auto rect=[&](QRectF r,QColor c,qreal radius=3){p.setBrush(c);p.drawRoundedRect(r,radius,radius);};
        const QColor teal("#0d9488"),blue("#4285d4"),paper("#e4eefb"),ink("#345477");
        switch(symbol){
        case Symbol::NewProject:{rect({6,3,20,26},blue);rect({8,5,16,22},paper,2);rect({11,10,8,2},blue,1);rect({11,14,6,2},blue,1);p.setBrush(teal);p.drawEllipse(QPointF(24,24),7,7);rect({20,23,8,2},Qt::white,1);rect({23,20,2,8},Qt::white,1);break;}
        case Symbol::Folder:{rect({3,7,13,8},QColor("#d99a24"));rect({3,11,26,17},QColor("#e8ae36"));QPainterPath front;front.moveTo(3,16);front.lineTo(29,13);front.lineTo(27,28);front.lineTo(5,28);front.closeSubpath();p.fillPath(front,QColor("#f5c85b"));break;}
        case Symbol::Save:{rect({4,3,24,26},blue);rect({9,3,14,10},paper,1);rect({18,5,3,6},ink,0);rect({9,18,14,11},paper,1);rect({12,21,8,2},blue,1);rect({12,25,8,1},blue,0);break;}
        case Symbol::Home:{QPainterPath roof;roof.moveTo(2,14);roof.lineTo(16,3);roof.lineTo(30,14);roof.closeSubpath();p.fillPath(roof,blue);rect({7,13,18,16},paper,2);rect({13,20,6,9},teal,1);break;}
        case Symbol::Settings:{p.translate(16,16);p.setBrush(blue);for(int i=0;i<8;++i){p.drawRoundedRect(QRectF(-3,-14,6,9),1,1);p.rotate(45);}p.drawEllipse(QPointF(0,0),11,11);p.setBrush(paper);p.drawEllipse(QPointF(0,0),5,5);break;}
        case Symbol::Tutorial:{rect({3,5,26,23},blue);rect({6,8,9,17},paper,1);rect({17,8,9,17},paper,1);rect({8,11,5,2},teal,0);rect({19,11,5,2},teal,0);break;}
        case Symbol::Circuit:{rect({6,6,20,20},teal);p.setPen(QPen(ink,2));for(int i:{10,16,22}){p.drawLine(i,2,i,6);p.drawLine(i,26,i,30);p.drawLine(2,i,6,i);p.drawLine(26,i,30,i);}p.setPen(Qt::NoPen);rect({11,11,10,10},paper,1);break;}
        case Symbol::Reset:{p.setPen(QPen(blue,3,Qt::SolidLine,Qt::RoundCap));p.setBrush(Qt::NoBrush);p.drawArc(QRectF(6,6,20,20),35*16,285*16);QPainterPath arrow;arrow.moveTo(20,3);arrow.lineTo(29,5);arrow.lineTo(25,13);arrow.closeSubpath();p.fillPath(arrow,blue);break;}
        case Symbol::Play:{QPainterPath path;path.moveTo(10,6);path.lineTo(26,16);path.lineTo(10,26);path.closeSubpath();p.fillPath(path,Qt::white);break;}
        case Symbol::Stop:rect({7,7,18,18},Qt::white,3);break;
        }
        p.end();result.addPixmap(image);
    }
    return result;
}
