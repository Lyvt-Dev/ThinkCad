#include "miditools.hpp"
#include "pianokeyboard.hpp"
#include "language.hpp"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QPlainTextEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QPushButton>
#include <algorithm>
MidiTools::MidiTools(QWidget* parent):QWidget(parent){setObjectName("midiTools");auto* layout=new QVBoxLayout(this);monitor_=new QPlainTextEdit;monitor_->setReadOnly(true);monitor_->setMaximumBlockCount(1000);monitor_->setObjectName("midiMonitor");layout->addWidget(monitor_);auto* form=new QFormLayout;channel_=new QSpinBox;channel_->setRange(0,16);channel_->setSpecialValueText(Language::text("Unverändert"));transpose_=new QSpinBox;transpose_->setRange(-48,48);mode_=new QComboBox;mode_->addItems({Language::text("MIDI weitergeben"),Language::text("Note → Control Change")});controller_=new QSpinBox;controller_->setRange(0,127);controller_->setValue(1);form->addRow(Language::text("Zielkanal (0 = Original)"),channel_);form->addRow(Language::text("Transponieren (Halbtöne)"),transpose_);form->addRow(Language::text("Übersetzung"),mode_);form->addRow(Language::text("CC-Nummer"),controller_);layout->addLayout(form);auto* send=new QPushButton(Language::text("Letztes MIDI-Ereignis übersetzen und senden"));send->setObjectName("sendTranslatedMidi");layout->addWidget(send);connect(send,&QPushButton::clicked,this,[this]{const auto bytes=translate(last_,channel_->value(),transpose_->value(),mode_->currentIndex()==1,controller_->value());if(!bytes.isEmpty())emit sendPacket(bytes);});}
QByteArray MidiTools::translate(const thinkermidi::Message& m,int channel,int transpose,bool toCc,int cc){
    if(!m.status)return {};if(!m.sysex.empty())return QByteArray(reinterpret_cast<const char*>(m.sysex.data()),m.sysex.size());
    QByteArray bytes;int status=m.status,a=m.data1,b=m.data2;
    if(status<0xf0){const int type=status&0xf0;if(channel>0)status=(status&0xf0)|((channel-1)&15);
        if(type==0x80||type==0x90){if(toCc){status=(status&15)|0xb0;a=std::clamp(cc,0,127);b=type==0x80?0:b;}else a=std::clamp(a+transpose,0,127);}
        bytes.append(char(status));bytes.append(char(a));if((status&0xf0)!=0xc0&&(status&0xf0)!=0xd0)bytes.append(char(b));
    }else{bytes.append(char(status));if(status==0xf1||status==0xf2||status==0xf3)bytes.append(char(a));if(status==0xf2)bytes.append(char(b));}return bytes;
}
void MidiTools::receive(const QString& source,const QByteArray& data){for(unsigned char byte:data){thinkermidi::Message m;if(!parsers_[source].feed(byte,m))continue;last_=m;const int type=m.status&0xf0;QString text;if(type==0x80||type==0x90)text=QString("%1 %2 · %3 Hz · velocity %4").arg(type==0x80||!m.data2?"Note Off":"Note On",PianoKeyboard::noteName(m.data1)).arg(PianoKeyboard::frequency(m.data1),0,'f',3).arg(m.data2);else if(type==0xb0)text=QString("CC %1 = %2").arg(m.data1).arg(m.data2);else if(type==0xe0)text=QString("Pitch Bend %1").arg(((m.data2<<7)|m.data1)-8192);else text=QString("MIDI 0x%1 · %2 · %3").arg(m.status,2,16,QChar('0')).arg(m.data1).arg(m.data2);monitor_->appendPlainText(source+QString(" · CH %1 · ").arg(m.status<0xf0?(m.status&15)+1:0)+text);}}
