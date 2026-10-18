#pragma once
#include <QAbstractButton>
#include <QApplication>
#include <QEvent>
#include <QEasingCurve>
#include <QPainter>
#include <QSettings>
#include <QVariantAnimation>
namespace Motion {
inline bool enabled(){return QSettings().value("animationsEnabled",true).toBool();}
// A mouse-transparent overlay leaves native styles, focus, layout and hit targets intact.
class ButtonOverlay final:public QWidget {
    QAbstractButton* button_;QVariantAnimation animation_;double amount_=0;
public:
    explicit ButtonOverlay(QAbstractButton* button):QWidget(button),button_(button){setObjectName("buttonMotionOverlay");setAttribute(Qt::WA_TransparentForMouseEvents);setAttribute(Qt::WA_NoSystemBackground);setAttribute(Qt::WA_TranslucentBackground);setFocusPolicy(Qt::NoFocus);setGeometry(button->rect());button->installEventFilter(this);animation_.setDuration(130);animation_.setEasingCurve(QEasingCurve::OutCubic);QObject::connect(&animation_,&QVariantAnimation::valueChanged,this,[this](const QVariant& v){amount_=v.toDouble();update();});show();}
    bool eventFilter(QObject*,QEvent* e)override{if(e->type()==QEvent::Resize){setGeometry(button_->rect());raise();}if(e->type()==QEvent::Enter||e->type()==QEvent::Leave||e->type()==QEvent::MouseButtonPress||e->type()==QEvent::MouseButtonRelease||e->type()==QEvent::EnabledChange){animation_.stop();if(!enabled()||!button_->isEnabled()){amount_=0;update();return false;}animation_.setStartValue(amount_);animation_.setEndValue(e->type()==QEvent::Leave?0.:e->type()==QEvent::MouseButtonPress?1.:.45);animation_.start();}return false;}
    void paintEvent(QPaintEvent*)override{if(!enabled()||amount_<=0)return;QPainter p(this);p.setRenderHint(QPainter::Antialiasing);p.setPen(QPen(QColor(20,160,155,int(90*amount_)),1));p.setBrush(QColor(20,160,155,int(24*amount_)));p.drawRoundedRect(rect().adjusted(1,1,-2,-2),5,5);}
};
class Installer final:public QObject {
public:
    explicit Installer(QObject* parent):QObject(parent){}
    bool eventFilter(QObject* object,QEvent* event)override{if(event->type()==QEvent::Show){if(auto* button=qobject_cast<QAbstractButton*>(object);button&&!button->property("motionInstalled").toBool()){button->setProperty("motionInstalled",true);new ButtonOverlay(button);}}return false;}
};
inline void install(){static bool installed=false;if(installed||!qApp)return;installed=true;qApp->installEventFilter(new Installer(qApp));}
}
