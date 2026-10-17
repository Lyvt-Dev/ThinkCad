#pragma once
#include "Arduino.h"
#include "MidiParser.h"
constexpr uint8_t MIDI_CHANNEL_OMNI=0,MIDI_CHANNEL_OFF=17;
namespace midi {
enum MidiType:uint8_t {InvalidType=0,NoteOff=0x80,NoteOn=0x90,AfterTouchPoly=0xa0,ControlChange=0xb0,ProgramChange=0xc0,AfterTouchChannel=0xd0,PitchBend=0xe0,SystemExclusive=0xf0,TimeCodeQuarterFrame=0xf1,SongPosition=0xf2,SongSelect=0xf3,TuneRequest=0xf6,Clock=0xf8,Start=0xfa,Continue=0xfb,Stop=0xfc,ActiveSensing=0xfe,SystemReset=0xff};
namespace Thru {enum Mode {Off,Full,SameChannel,DifferentChannel};}
class MidiInterface {
public:
    explicit MidiInterface(SerialPort& port):port_(port){}
    void begin(uint8_t channel=1){if(channel>17)throw std::invalid_argument("MIDI input channel must be 0..17");channel_=channel;port_.begin(31250);parser_={};last_={};}
    void send(MidiType type,uint8_t d1,uint8_t d2,uint8_t channel){if(type<0x80||type>=0xf0||channel<1||channel>16||d1>127||d2>127)throw std::invalid_argument("Invalid MIDI channel message");uint8_t bytes[]={uint8_t(type|((channel-1)&15)),d1,d2};port_.write(bytes,type==ProgramChange||type==AfterTouchChannel?2:3);}
    void sendNoteOn(uint8_t note,uint8_t velocity,uint8_t channel){send(NoteOn,note,velocity,channel);}
    void sendNoteOff(uint8_t note,uint8_t velocity,uint8_t channel){send(NoteOff,note,velocity,channel);}
    void sendControlChange(uint8_t controller,uint8_t value,uint8_t channel){send(ControlChange,controller,value,channel);}
    void sendProgramChange(uint8_t number,uint8_t channel){send(ProgramChange,number,0,channel);}
    void sendAfterTouch(uint8_t value,uint8_t channel){send(AfterTouchChannel,value,0,channel);}
    void sendAfterTouch(uint8_t note,uint8_t value,uint8_t channel){send(AfterTouchPoly,note,value,channel);}
    void sendPitchBend(int value,uint8_t channel){value=std::clamp(value,-8192,8191)+8192;send(PitchBend,value&127,(value>>7)&127,channel);}
    void sendRealTime(MidiType type){if(type<Clock)throw std::invalid_argument("Not a MIDI realtime message");port_.write(uint8_t(type));}
    void sendClock(){sendRealTime(Clock);}void sendStart(){sendRealTime(Start);}void sendStop(){sendRealTime(Stop);}void sendContinue(){sendRealTime(Continue);}
    void sendSysEx(unsigned length,const uint8_t* data,bool boundaries=false){if(!data||length>1024)throw std::invalid_argument("Invalid MIDI SysEx");if(!boundaries)port_.write(uint8_t(0xf0));port_.write(data,length);if(!boundaries)port_.write(uint8_t(0xf7));}
    bool read(){while(port_.available()){const int byte=port_.read();if(byte<0)return false;thinkermidi::Message message;if(!parser_.feed(byte,message))continue;
        const int ch=message.status<0xf0?(message.status&15)+1:0;const bool accepted=channel_!=MIDI_CHANNEL_OFF&&(!ch||channel_==0||channel_==ch);
        if(thru_!=Thru::Off&&(thru_==Thru::Full||(thru_==Thru::SameChannel&&accepted)||(thru_==Thru::DifferentChannel&&!accepted)))forward(message);
        if(!accepted)continue;last_=message;const auto type=getType();if(type==NoteOn&&message.data2&&noteOn_)noteOn_(ch,message.data1,message.data2);else if((type==NoteOff||(type==NoteOn&&!message.data2))&&noteOff_)noteOff_(ch,message.data1,message.data2);else if(type==ControlChange&&control_)control_(ch,message.data1,message.data2);else if(type==PitchBend&&bend_)bend_(ch,((message.data2<<7)|message.data1)-8192);return true;
    }return false;}
    MidiType getType()const{return MidiType(last_.status<0xf0?last_.status&0xf0:last_.status);}uint8_t getChannel()const{return last_.status>=0x80&&last_.status<0xf0?(last_.status&15)+1:0;}uint8_t getData1()const{return last_.data1;}uint8_t getData2()const{return last_.data2;}
    const uint8_t* getSysExArray()const{return last_.sysex.data();}unsigned getSysExArrayLength()const{return last_.sysex.size();}
    void turnThruOff(){thru_=Thru::Off;}void turnThruOn(Thru::Mode mode=Thru::Full){thru_=mode;}void setThruFilterMode(Thru::Mode mode){thru_=mode;}
    void setHandleNoteOn(void (*callback)(uint8_t,uint8_t,uint8_t)){noteOn_=callback;}void setHandleNoteOff(void (*callback)(uint8_t,uint8_t,uint8_t)){noteOff_=callback;}void setHandleControlChange(void (*callback)(uint8_t,uint8_t,uint8_t)){control_=callback;}void setHandlePitchBend(void (*callback)(uint8_t,int)){bend_=callback;}
private:
    SerialPort& port_;uint8_t channel_=1;Thru::Mode thru_=Thru::Full;thinkermidi::Parser parser_;thinkermidi::Message last_;
    void (*noteOn_)(uint8_t,uint8_t,uint8_t)=nullptr;void (*noteOff_)(uint8_t,uint8_t,uint8_t)=nullptr;void (*control_)(uint8_t,uint8_t,uint8_t)=nullptr;void (*bend_)(uint8_t,int)=nullptr;
    void forward(const thinkermidi::Message& m){if(!m.sysex.empty())port_.write(m.sysex.data(),m.sysex.size());else if(m.status>=0xf0){port_.write(m.status);if(m.status==0xf1||m.status==0xf2||m.status==0xf3)port_.write(m.data1);if(m.status==0xf2)port_.write(m.data2);}else send(MidiType(m.status&0xf0),m.data1,m.data2,(m.status&15)+1);}
};
}
#define MIDI_CREATE_DEFAULT_INSTANCE() inline midi::MidiInterface MIDI(Serial)
#define MIDI_CREATE_INSTANCE(Type, Port, Name) inline midi::MidiInterface Name(Port)
