#pragma once
#include <QPushButton>
#include <QMouseEvent>
#include <QApplication>
#include <QDrag>
#include <QMimeData>
#include <QJsonDocument>
#include <QJsonObject>
// The palette is a drag source. A click never edits the document.
class ComponentPaletteButton final:public QPushButton {
    QPoint origin_;
public:
    explicit ComponentPaletteButton(const QString& text):QPushButton(text){}
protected:
    void mousePressEvent(QMouseEvent* e)override{origin_=e->pos();QPushButton::mousePressEvent(e);}
    void mouseMoveEvent(QMouseEvent* e)override{
        if(!(e->buttons()&Qt::LeftButton)||(e->pos()-origin_).manhattanLength()<QApplication::startDragDistance()){QPushButton::mouseMoveEvent(e);return;}
        setDown(false);QDrag drag(this);auto* mime=new QMimeData;
        mime->setData("application/x-thinkcad-component",QJsonDocument(QJsonObject{{"kind",property("kind").toInt()},{"model",property("model").toString()}}).toJson(QJsonDocument::Compact));
        drag.setMimeData(mime);drag.setPixmap(grab());drag.setHotSpot(origin_);drag.exec(Qt::CopyAction);
    }
};
