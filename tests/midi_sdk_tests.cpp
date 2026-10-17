#include "MIDI.h"
#include <stdexcept>
#include <iostream>
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
class Port:public SerialPort {
public:
    using SerialPort::write;std::vector<uint8_t> in,out;std::size_t at=0;
    int available()override{return in.size()-at;}int read()override{return available()?in[at++]:-1;}int peek()override{return available()?in[at]:-1;}
    std::size_t write(uint8_t b)override{out.push_back(b);return 1;}
    std::size_t write(const uint8_t* b,std::size_t n)override{out.insert(out.end(),b,b+n);return n;}
};
int main(){try{Port port;midi::MidiInterface midi(port);midi.begin(MIDI_CHANNEL_OMNI);midi.turnThruOff();midi.sendNoteOn(60,100,2);midi.sendControlChange(7,127,1);midi.sendPitchBend(-8192,1);check(port.out==std::vector<uint8_t>({0x91,60,100,0xb0,7,127,0xe0,0,0}),"MIDI encodes channel, notes, CC and signed bend");
port.in={0x92,60,0xf8,100,61,0,0xf0,1,2,0xfa,3,0xf7};check(midi.read()&&midi.getType()==midi::Clock,"realtime inside channel message");check(midi.read()&&midi.getType()==midi::NoteOn&&midi.getData1()==60&&midi.getData2()==100&&midi.getChannel()==3,"complete channel message after realtime");check(midi.read()&&midi.getData1()==61&&midi.getData2()==0,"running status");check(midi.read()&&midi.getType()==midi::Start,"realtime inside SysEx");check(midi.read()&&midi.getType()==midi::SystemExclusive&&midi.getSysExArrayLength()==5,"bounded SysEx preserved");
port.in={0x91,40,50,0x90,41,51};port.at=0;port.out.clear();midi.begin(1);midi.turnThruOn(midi::Thru::DifferentChannel);check(midi.read()&&midi.getData1()==41,"input channel filter");check(port.out==std::vector<uint8_t>({0x91,40,50}),"different-channel thru");thinkermidi::Parser parser;thinkermidi::Message message;parser.feed(0xf0,message);for(int i=0;i<1030;++i)parser.feed(1,message);check(!parser.feed(0xf7,message),"oversized SysEx rejected");parser.feed(0x90,message);parser.feed(60,message);check(parser.feed(1,message)&&message.data1==60,"parser recovers after oversized SysEx");std::cout<<"MIDI SDK parsing, realtime, running status, SysEx, filters and transport passed\n";return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
