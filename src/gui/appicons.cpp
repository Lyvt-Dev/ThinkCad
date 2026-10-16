#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include "appicons.hpp"
#include <QApplication>
#include <QStyle>
QIcon AppIcons::icon(Symbol symbol){
    if(symbol==Symbol::Home||symbol==Symbol::NewProject||symbol==Symbol::Folder||symbol==Symbol::Save||symbol==Symbol::Pause||symbol==Symbol::Step||symbol==Symbol::Wire||symbol==Symbol::Palette){QPixmap pixmap(48,48);pixmap.fill(Qt::transparent);QPainter p(&pixmap);p.setRenderHint(QPainter::Antialiasing);p.scale(2,2);p.setPen(QPen(QColor("#168a9e"),1.8,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));p.setBrush(Qt::NoBrush);
        if(symbol==Symbol::Home){QPainterPath path;path.moveTo(3,11);path.lineTo(12,3);path.lineTo(21,11);p.drawPath(path);path=QPainterPath();path.moveTo(5,10);path.lineTo(5,21);path.lineTo(10,21);path.lineTo(10,15);path.lineTo(14,15);path.lineTo(14,21);path.lineTo(19,21);path.lineTo(19,10);p.drawPath(path);}
        else if(symbol==Symbol::Folder){QPainterPath path;path.moveTo(3,19);path.lineTo(3,6);path.lineTo(9,6);path.lineTo(11,9);path.lineTo(21,9);path.lineTo(19,19);path.closeSubpath();p.drawPath(path);p.drawLine(4,12,19,12);}
        else if(symbol==Symbol::Save){p.drawRoundedRect(QRectF(3,3,18,18),4,4);p.drawLine(12,6,12,15);p.drawLine(8,11,12,15);p.drawLine(12,15,16,11);p.drawLine(7,18,17,18);}
        else if(symbol==Symbol::NewProject){p.drawRoundedRect(QRectF(5,3,14,18),2,2);p.drawLine(8,12,16,12);p.drawLine(12,8,12,16);}
        else if(symbol==Symbol::Pause){p.drawLine(8,5,8,19);p.drawLine(16,5,16,19);}
        else if(symbol==Symbol::Step){p.drawLine(18,5,18,19);QPainterPath path;path.moveTo(5,5);path.lineTo(14,12);path.lineTo(5,19);path.closeSubpath();p.drawPath(path);}
        else if(symbol==Symbol::Wire){p.drawEllipse(QPointF(4,18),2,2);p.drawEllipse(QPointF(20,6),2,2);p.drawLine(6,18,12,18);p.drawLine(12,18,12,6);p.drawLine(12,6,18,6);}
        else {p.drawEllipse(QPointF(12,12),9,9);for(auto c:{QColor("#e95a60"),QColor("#eab442"),QColor("#16a085")}){p.setBrush(c);p.setPen(Qt::NoPen);p.drawEllipse(QPointF(c.red()>220?7:16,c.green()>150?8:15),2.8,2.8);}}
        return QIcon(pixmap);}
    QStyle::StandardPixmap image=QStyle::SP_FileIcon;
    switch(symbol){
    case Symbol::NewProject:image=QStyle::SP_FileIcon;break;
    case Symbol::Folder:image=QStyle::SP_DirOpenIcon;break;
    case Symbol::Save:image=QStyle::SP_DialogSaveButton;break;
    case Symbol::Home:image=QStyle::SP_DirHomeIcon;break;
    case Symbol::Settings:image=QStyle::SP_FileDialogDetailedView;break;
    case Symbol::Tutorial:image=QStyle::SP_DialogHelpButton;break;
    case Symbol::Circuit:image=QStyle::SP_FileIcon;break;
    case Symbol::Reset:image=QStyle::SP_BrowserReload;break;
    case Symbol::Play:image=QStyle::SP_MediaPlay;break;
    default:break;
    case Symbol::Stop:image=QStyle::SP_MediaStop;break;
    }
    return QApplication::style()->standardIcon(image);
}
