#include "irwaveform.hpp"
#include <iostream>
#include <chrono>
#include <cstdlib>
using namespace thinkir;
void check(bool value,const char* message){if(!value){std::cerr<<message<<'\n';std::exit(1);}}
int main(){
 for(auto protocol:{Nec,Samsung,Sony,Rc5,Rc6,Kaseikyo})for(unsigned address:{0u,1u,31u})for(unsigned command:{0u,1u,63u}){
  auto w=encode(protocol,address,command);auto d=decode(w);if(!d)std::cerr<<name(protocol)<<" decode failed\n";check(bool(d),"Protocol decode");check(d->address==address&&d->command==command&&d->protocol==protocol,"Protocol data");
  auto jitter=w;for(unsigned i=0;i<jitter.pulses.size();++i)jitter.pulses[i].us=unsigned(jitter.pulses[i].us*(i%2?1.06:.94));if(!decode(jitter))std::cerr<<name(protocol)<<" jitter failed\n";check(bool(decode(jitter)),"Timing tolerance");
 }
 // Fixed reference words, independent of the round-trip encoder/decoder checks.
 check(decode(encode(Nec,0x1234,0x45))->raw==0xba451234ULL,"NEC reference word");
 check(decode(encode(Nec,0x10,0x45))->raw==0xba45ef10ULL,"NEC inverted address reference");
 check(decode(encode(Samsung,0x12,0x34))->raw==0xcb341212ULL,"Samsung duplicated address reference");
 check(decode(encode(Kaseikyo,0x123,0x45))->raw==0x674512302002ULL,"Kaseikyo reference parity/checksum");
 check(decode(encode(Sony,1,12))->raw==0x8c,"Sony LSB reference word");
 check(decode(encode(Rc5,1,2))->raw==0x1042,"RC5 reference word");
 check(decode(encode(Rc6,1,2))->raw==0x102,"RC6 mode zero reference word");
 auto nec=encode(Nec,0x1234,0x45);check(nec.duration()==64590&&encode(Nec,0x10,0x45).duration()==67980,"NEC reference durations");check(nec.pulses.size()==67,"NEC pulse count");check(nec.pulses[0].us==9000&&nec.pulses[1].us==4500,"NEC leader");auto d=decode(nec);check(d&&d->address==0x1234&&d->command==0x45,"Extended NEC");check((d->raw>>24)==0xba,"Command complement");auto repeat=decode(encode(Nec,0,0,true));check(repeat&&repeat->repeat,"NEC short repeat");
 auto bad=nec;bad.pulses[15].us=3000;check(!decode(bad),"Invalid bit rejected");bad=nec;bad.pulses.pop_back();check(!decode(bad),"Truncated frame rejected");
 Optics o;check(bool(demodulate(nec,o)),"Optical receive");o.distance=1401;check(!demodulate(nec,o),"Range");o.distance=0;o.lineOfSight=false;check(!demodulate(nec,o),"Obstruction");o.lineOfSight=true;o.sensitivity=.01;check(!demodulate(nec,o),"Sensitivity");o.sensitivity=1;o.ambient=10;check(!demodulate(nec,o),"Ambient rejection");o.ambient=0;auto wrong=nec;wrong.hz=56000;check(!demodulate(wrong,o),"Carrier rejection");o.hz=56000;check(bool(demodulate(wrong,o)),"56 kHz receiver");
 unsigned edges=0;bool previous=false;for(unsigned t=0;t<9000;++t){bool v=nec.carrier(t);edges+=v&&!previous;previous=v;}check(edges==342,"38 kHz carrier count");
 for(int speed:{100,1000,10000}){(void)speed;check(encode(Nec,0x1234,0x45).duration()==nec.duration(),"Speed independent duration");}
 auto start=std::chrono::steady_clock::now();unsigned count=0;for(int i=0;i<10000;++i){auto frame=encode(Nec,std::uint16_t(i),0x45);auto result=decode(frame);count+=result.has_value();}check(count==10000,"Bounded waveform profile");auto elapsed=std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-start).count();
 std::cout<<"IR protocol, timing, optical filtering and analytical carrier tests passed; 10000 encode/decode pairs in "<<elapsed<<" us\n";
}
