#include "pianokeyboard.hpp"
#include <QPainter>
#include <QMouseEvent>
#include <cmath>
namespace {bool black(int n){const int p=n%12;return p==1||p==3||p==6||p==8||p==10;}}
PianoKeyboard::PianoKeyboard(int first,int last,QWidget* parent):QWidget(parent),first_(first),last_(last){int count=0;for(int n=first;n<=last;++n)count+=!black(n);setFixedSize(count*24+1,166);setMouseTracking(true);setFocusPolicy(Qt::StrongFocus);setObjectName("pianoKeyboard");setAccessibleName(QString("Piano %1 keys").arg(last-first+1));}
double PianoKeyboard::frequency(int n){return 440*std::pow(2.,(n-69)/12.);}
QString PianoKeyboard::noteName(int n){static const char* names[]={"C","C♯","D","D♯","E","F","F♯","G","G♯","A","A♯","B"};return QString::fromUtf8(names[n%12])+QString::number(n/12-1);}
QRectF PianoKeyboard::keyRect(int note)const{if(note<first_||note>last_)return {};int whites=0;for(int n=first_;n<note;++n)whites+=!black(n);return black(note)?QRectF(whites*24-7,0,14,100):QRectF(whites*24,0,24,165);}
int PianoKeyboard::noteAt(QPointF point)const{for(int n=first_;n<=last_;++n)if(black(n)&&keyRect(n).contains(point))return n;for(int n=first_;n<=last_;++n)if(!black(n)&&keyRect(n).contains(point))return n;return -1;}
void PianoKeyboard::paintEvent(QPaintEvent*){QPainter p(this);p.setRenderHint(QPainter::Antialiasing);for(bool blacks:{false,true})for(int n=first_;n<=last_;++n)if(black(n)==blacks){const auto r=keyRect(n);p.setBrush(n==held_?QColor("#22b8b0"):QColor(blacks?"#202b36":"#f5f7fa"));p.setPen(QColor("#6b7785"));p.drawRoundedRect(r.adjusted(.5,.5,-.5,-.5),2,2);if(!blacks){p.setPen(QColor("#425263"));p.setFont(QFont("Sans Serif",7));p.drawText(r.adjusted(1,130,-1,-3),Qt::AlignCenter,n%12==0||n==first_||n==last_?noteName(n):QString());}}}
void PianoKeyboard::hold(int note){if(note==held_)return;if(held_>=0)emit noteReleased(held_);held_=note;if(note>=0)emit notePressed(note);update();}
void PianoKeyboard::mousePressEvent(QMouseEvent* e){if(e->button()==Qt::LeftButton){setFocus();hold(noteAt(e->position()));e->accept();}}
void PianoKeyboard::mouseMoveEvent(QMouseEvent* e){if(e->buttons()&Qt::LeftButton)hold(noteAt(e->position()));}
void PianoKeyboard::mouseReleaseEvent(QMouseEvent*){hold(-1);}
void PianoKeyboard::focusOutEvent(QFocusEvent*){hold(-1);}
