#include "tonepiano.hpp"
#include "pianokeyboard.hpp"
#include "language.hpp"
#include <QVBoxLayout>
#include <QLabel>
#include <QScrollArea>
TonePiano::TonePiano(QWidget* parent):QDialog(parent){setObjectName("tonePiano");setWindowTitle(Language::text("Piezo-Piano · 88 Tasten"));setAttribute(Qt::WA_DeleteOnClose);resize(1150,285);auto* layout=new QVBoxLayout(this);auto* label=new QLabel(Language::text("Taste drücken · A0–C8 · A4 = 440 Hz"));label->setObjectName("pianoFrequency");label->setTextInteractionFlags(Qt::TextSelectableByMouse);layout->addWidget(label);auto* scroll=new QScrollArea;scroll->setWidgetResizable(false);auto* keys=new PianoKeyboard;scroll->setWidget(keys);scroll->setMinimumHeight(195);layout->addWidget(scroll);auto* warning=new QLabel;warning->setWordWrap(true);layout->addWidget(warning);connect(&sound_,&SoundOutput::notice,warning,&QLabel::setText);connect(keys,&PianoKeyboard::notePressed,this,[this,label](int n){const auto f=PianoKeyboard::frequency(n);label->setText(QString("%1 · %2 Hz · tone(pin, %3); · MIDI %4").arg(PianoKeyboard::noteName(n)).arg(f,0,'f',3).arg(qRound(f)).arg(n));sound_.setSignals({{"piano",f,.65}});});connect(keys,&PianoKeyboard::noteReleased,this,[this](int){sound_.stop();});}

TonePiano::~TonePiano(){for(auto* keys:findChildren<PianoKeyboard*>())keys->disconnect(this);sound_.stop();}
