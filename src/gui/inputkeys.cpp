#include "inputkeys.hpp"
#include "pianokeyboard.hpp"
#include "language.hpp"
#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QSpinBox>
#include <QCloseEvent>
#include <QTimer>
#include <QDoubleSpinBox>
#include <QSlider>
#include <QSignalBlocker>
using namespace thinkercad;
InputKeys::InputKeys(const CircuitPart& part,QWidget* parent):QDialog(parent){setObjectName("componentKeys");setAttribute(Qt::WA_DeleteOnClose);setWindowTitle(Language::text(QString::fromStdString(componentSpec(part.kind).label))+" · "+QString::fromStdString(part.id));auto* layout=new QVBoxLayout(this);auto* hint=new QLabel(Language::text("Gedrückt halten: verbindet die zugehörige Zeile mit der Spalte. INPUT_PULLUP verwenden; ohne Isolierdioden kann Ghosting auftreten."));if(part.kind==PartKind::RotaryEncoder)hint->setText(Language::text("Ein Klick erzeugt vier Quadratur-Flanken. A/B und SW mit INPUT_PULLUP lesen; GND mit Board-Masse verbinden."));hint->setWordWrap(true);layout->addWidget(hint);if(part.kind==PartKind::RotaryEncoder){encoder_=true;auto* row=new QHBoxLayout;auto* left=new QPushButton("↶");auto* right=new QPushButton("↷");left->setObjectName("encoderLeft");right->setObjectName("encoderRight");left->setMinimumHeight(55);right->setMinimumHeight(55);row->addWidget(left);row->addWidget(right);layout->addLayout(row);auto* push=new QPushButton(Language::text("Encoder-Taster"));layout->addWidget(push);connect(push,&QPushButton::pressed,this,[this]{emit encoderSwitch(true);});connect(push,&QPushButton::released,this,[this]{emit encoderSwitch(false);});auto rotate=[this,left,right](int direction){emit encoderTurn(direction);left->setEnabled(false);right->setEnabled(false);auto* timer=new QTimer(this);auto step=std::make_shared<int>(0);connect(timer,&QTimer::timeout,this,[this,timer,step,direction,left,right]{++*step;emit encoderPhase((direction*(*step)+4)%4);if(*step==4){timer->stop();timer->deleteLater();left->setEnabled(true);right->setEnabled(true);}});timer->start(20);};connect(left,&QPushButton::clicked,this,[rotate]{rotate(-1);});connect(right,&QPushButton::clicked,this,[rotate]{rotate(1);});}
else if(part.kind==PartKind::Keypad){auto* grid=new QGridLayout;const QStringList names{"1","2","3","A","4","5","6","B","7","8","9","C","*","0","#","D"};for(int i=0;i<16;++i){auto* key=new QPushButton(names[i]);key->setMinimumSize(54,46);key->setObjectName("matrixKey"+QString::number(i));key->setToolTip(QString("R%1 ↔ C%2").arg(i/4+1).arg(i%4+1));grid->addWidget(key,i/4,i%4);connect(key,&QPushButton::pressed,this,[this,i]{setKey(i,true);});connect(key,&QPushButton::released,this,[this,i]{setKey(i,false);});}layout->addLayout(grid);}else if(part.kind==PartKind::Keyboard){
    auto* send=new QCheckBox(Language::text("MIDI an den seriellen Eingang des Sketches senden"));send->setObjectName("keyboardMidiEnabled");layout->addWidget(send);auto* channel=new QSpinBox;channel->setRange(1,16);channel->setPrefix("MIDI CH ");layout->addWidget(channel);auto* keys=new PianoKeyboard(48,72);layout->addWidget(keys);
    auto midi=[this,send,channel](int note,bool on){int ch=channel->value();if(on){if(!send->isChecked())return;midiHeld_[note]=ch;}else{auto it=midiHeld_.find(note);if(it==midiHeld_.end())return;ch=it->second;midiHeld_.erase(it);}QByteArray packet;packet.append(char((on?0x90:0x80)|(ch-1)));packet.append(char(note));packet.append(char(on?100:0));emit midiPacket(packet);};
    connect(keys,&PianoKeyboard::notePressed,this,[this,midi](int note){setKey(note-48,true);midi(note,true);});connect(keys,&PianoKeyboard::noteReleased,this,[this,midi](int note){setKey(note-48,false);midi(note,false);});
}else{
    hint->setText(Language::text("Wert während der Simulation ändern."));auto* value=new QDoubleSpinBox;value->setObjectName("inputValue");const auto& spec=componentSpec(part.kind);value->setRange(spec.min,spec.max);value->setDecimals(part.kind==PartKind::Potentiometer?1:3);value->setSuffix(Language::text(QString::fromStdString(spec.unit)));value->setValue(part.value);layout->addWidget(value);
    if(part.kind==PartKind::Potentiometer){auto* slider=new QSlider(Qt::Horizontal);slider->setRange(0,100);slider->setValue(qRound(part.value));slider->setObjectName("analogInputSlider");layout->addWidget(slider);connect(slider,&QSlider::valueChanged,value,[value](int v){value->setValue(v);});connect(value,qOverload<double>(&QDoubleSpinBox::valueChanged),slider,[slider](double v){QSignalBlocker block(slider);slider->setValue(qRound(v));});}
    connect(value,qOverload<double>(&QDoubleSpinBox::valueChanged),this,&InputKeys::valueChanged);
}resize(sizeHint());}
void InputKeys::setKey(int key,bool pressed){if(pressed){if(!held_.insert(key).second)return;}else if(!held_.erase(key))return;emit keyChanged(key,pressed);}
void InputKeys::releaseAll(){if(encoder_){emit encoderPhase(0);emit encoderSwitch(false);}const auto held=held_;for(int key:held)setKey(key,false);for(const auto& [note,ch]:midiHeld_){QByteArray packet;packet.append(char(0x80|(ch-1)));packet.append(char(note));packet.append(char(0));emit midiPacket(packet);}midiHeld_.clear();}
void InputKeys::closeEvent(QCloseEvent* e){for(auto* timer:findChildren<QTimer*>())timer->stop();releaseAll();QDialog::closeEvent(e);}
void InputKeys::changeEvent(QEvent* e){if(e->type()==QEvent::ActivationChange&&!isActiveWindow())releaseAll();QDialog::changeEvent(e);}
