#pragma once
// Compact carrier-burst representation. Durations are virtual microseconds;
// carrier edges are evaluated analytically, never scheduled as GUI frames.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <vector>
namespace thinkir {
enum Protocol { Unknown, Nec, Samsung, Sony, Rc5, Rc6, Kaseikyo };
inline const char* name(Protocol p){switch(p){case Nec:return "NEC";case Samsung:return "Samsung";case Sony:return "Sony SIRC";case Rc5:return "RC5";case Rc6:return "RC6";case Kaseikyo:return "Kaseikyo";default:return "Unknown";}}
struct Pulse {bool mark;std::uint32_t us;};
struct Wave {
 unsigned hz=38000;double duty=1./3;std::vector<Pulse> pulses;
 std::uint64_t duration()const{std::uint64_t t=0;for(auto p:pulses)t+=p.us;return t;}
 bool carrier(std::uint64_t us)const{for(auto p:pulses){if(us<p.us)return p.mark&&std::fmod(double(us)*hz/1000000.,1.)<duty;us-=p.us;}return false;}
 std::uint64_t nextEdge(std::uint64_t us)const{
  std::uint64_t start=0;
  for(auto p:pulses){if(us<start+p.us){if(!p.mark)return start+p.us;const double cycle=double(us-start)*hz/1000000.;const double phase=cycle-std::floor(cycle);const double next=(std::floor(cycle)+(phase<duty-1e-10?duty:1))*1000000./hz;return std::min(start+p.us,std::max(us+1,start+std::uint64_t(std::ceil(next-1e-8))));}start+=p.us;}
  return 0;
 }
};

struct Decoded {Protocol protocol=Unknown;std::uint16_t address=0,command=0;std::uint64_t raw=0;unsigned bits=0;bool repeat=false;};
inline void pulse(Wave& w,bool mark,unsigned us){if(!us)return;if(!w.pulses.empty()&&w.pulses.back().mark==mark)w.pulses.back().us+=us;else w.pulses.push_back({mark,us});}
inline void distance(Wave& w,std::uint64_t raw,unsigned bits,unsigned mark,unsigned zero,unsigned one){for(unsigned b=0;b<bits;++b){pulse(w,true,mark);pulse(w,false,(raw>>b)&1?one:zero);}pulse(w,true,mark);}
inline Wave encode(Protocol p,std::uint16_t address,std::uint16_t command,bool repeat=false,unsigned bits=0,bool toggle=false,std::uint16_t vendor=0x2002){
 Wave w;
 if(p==Nec){if(command>255)throw std::invalid_argument("NEC command must fit in 8 bits");pulse(w,true,9000);pulse(w,false,repeat?2250:4500);if(repeat)pulse(w,true,560);else{auto hi=address>255?address>>8:(~address)&255;std::uint32_t raw=(address&255)|(hi<<8)|(std::uint32_t(command)<<16)|((~std::uint32_t(command)&255)<<24);distance(w,raw,32,560,560,1690);}}
 else if(p==Samsung){pulse(w,true,4500);pulse(w,false,4500);distance(w,std::uint32_t(address<256?address|(address<<8):address)|(std::uint32_t(command)<<16)|(command<256?((~std::uint32_t(command)&255)<<24):0),32,560,560,1690);}
 else if(p==Sony){w.hz=40000;if(!bits)bits=12;if(bits!=12&&bits!=15&&bits!=20)throw std::invalid_argument("Sony requires 12, 15 or 20 bits");if(command>127||address>=(1u<<(bits-7)))throw std::invalid_argument("Sony address/command exceeds frame width");pulse(w,true,2400);pulse(w,false,600);auto raw=std::uint32_t(command)|(std::uint32_t(address)<<7);for(unsigned b=0;b<bits;++b){pulse(w,true,(raw>>b)&1?1200:600);if(b+1<bits)pulse(w,false,600);}}
 else if(p==Rc5||p==Rc6){const bool six=p==Rc6;w.hz=36000;if(address>(six?255:31)||command>(six?255:127))throw std::invalid_argument("RC address/command exceeds frame width");unsigned half=six?444:889;std::uint32_t raw=six?((1u<<20)|(std::uint32_t(toggle)<<16)|(address<<8)|command):((1u<<13)|(std::uint32_t(command<64)<<12)|(std::uint32_t(toggle)<<11)|(address<<6)|(command&63));unsigned count=six?21:14;if(six){pulse(w,true,2666);pulse(w,false,889);}for(int b=int(count)-1;b>=0;--b){bool bit=(raw>>b)&1;unsigned width=six&&b==16?half*2:half; // RC6 trailer/toggle is double-width.
 // RC5 logical 1 is space->mark; RC6 logical 1 is mark->space.
 pulse(w,six?bit:!bit,width);pulse(w,six?!bit:bit,width);}}
 else if(p==Kaseikyo){if(address>4095||command>255)throw std::invalid_argument("Kaseikyo requires 12-bit address and 8-bit command");w.hz=37000;unsigned parity=(vendor^(vendor>>4)^(vendor>>8)^(vendor>>12))&15;std::uint64_t raw=vendor|(std::uint64_t(parity)<<16)|(std::uint64_t(address)<<20)|(std::uint64_t(command)<<32);unsigned checksum=((raw>>16)^(raw>>24)^(raw>>32))&255;raw|=std::uint64_t(checksum)<<40;pulse(w,true,3456);pulse(w,false,1728);distance(w,raw,48,432,432,1296);}
 else throw std::invalid_argument("Unsupported IR protocol");
 return w;
}
inline bool close(unsigned actual,unsigned expected,double tolerance=.22){return std::abs(double(actual)-expected)<=std::max(80.,expected*tolerance);}
inline std::optional<std::uint64_t> readDistance(const Wave& w,unsigned bits,unsigned leader,unsigned gap,unsigned unit,unsigned one){if(w.pulses.size()!=2+2*bits+1||!w.pulses[0].mark||w.pulses[1].mark||!close(w.pulses[0].us,leader)||!close(w.pulses[1].us,gap))return {};std::uint64_t raw=0;for(unsigned i=0;i<bits;++i){auto m=w.pulses[2+2*i],s=w.pulses[3+2*i];if(!m.mark||s.mark||!close(m.us,unit))return {};if(close(s.us,one))raw|=1ull<<i;else if(!close(s.us,unit))return {};}auto tail=w.pulses.back();if(!tail.mark||!close(tail.us,unit))return {};return raw;}
inline std::optional<Decoded> decode(const Wave& w){
 if(w.pulses.empty()||w.pulses.size()>256)return {};
 if(w.pulses.size()==3&&w.pulses[0].mark&&!w.pulses[1].mark&&w.pulses[2].mark&&close(w.pulses[0].us,9000)&&close(w.pulses[1].us,2250)&&close(w.pulses[2].us,560))return Decoded{Nec,0,0,0,0,true};
 for(auto p:{Nec,Samsung,Kaseikyo}){unsigned bits=p==Kaseikyo?48:32;auto raw=readDistance(w,bits,p==Nec?9000:p==Samsung?4500:3456,p==Nec||p==Samsung?4500:1728,p==Kaseikyo?432:560,p==Kaseikyo?1296:1690);if(!raw)continue;auto v=*raw;Decoded d{p,0,0,v,bits,false};if(p==Kaseikyo){unsigned vendor=v&65535,parity=(vendor^(vendor>>4)^(vendor>>8)^(vendor>>12))&15;if(((v>>16)&15)!=parity||((v>>40)&255)!=(((v>>16)^(v>>24)^(v>>32))&255))return {};d.address=(v>>20)&4095;d.command=(v>>32)&255;}else{if(p==Nec&&(((v>>16)^(v>>24))&255)!=255)return {};unsigned a=v&255,b=(v>>8)&255;d.address=(p==Nec&&(a^b)==255)||(p==Samsung&&a==b)?a:std::uint16_t(v);d.command=p==Samsung&&(((v>>16)^(v>>24))&255)!=255?std::uint16_t(v>>16):std::uint16_t((v>>16)&255);}return d;}
 // SIRC pulse-width encoding: trailing space is optional in receiver captures.
 auto n=w.pulses.size();if(n&& !w.pulses.back().mark)--n;unsigned bits=n>=2?unsigned((n-1)/2):0;
 auto sonyDecode=[&]()->std::optional<Decoded>{if((bits==12||bits==15||bits==20)&&w.pulses[0].mark&&close(w.pulses[0].us,2400,.08)){std::uint64_t raw=0;for(unsigned b=0;b<bits;++b){auto s=w.pulses[1+b*2],m=w.pulses[2+b*2];if(s.mark||!close(s.us,600)||!m.mark)return {};if(close(m.us,1200))raw|=1ull<<b;else if(!close(m.us,600))return {};}return Decoded{Sony,std::uint16_t(raw>>7),std::uint16_t(raw&127),raw,bits,false};}return {};};if(auto sony=sonyDecode())return sony;
 // Expand merged Manchester runs into half-bit cells, then validate both halves.
 for(auto p:{Rc5,Rc6}){bool six=p==Rc6;unsigned half=six?444:889;std::vector<bool> cells;std::size_t first=0;if(six){if(w.pulses.size()<3||!w.pulses[0].mark||w.pulses[1].mark||!close(w.pulses[0].us,2666)||!close(w.pulses[1].us,889))continue;first=2;}bool valid=true;for(auto i=first;i<w.pulses.size();++i){unsigned count=unsigned(std::llround(double(w.pulses[i].us)/half));if(!count||count>6||!close(w.pulses[i].us,count*half,.15)){valid=false;break;}for(unsigned j=0;j<count;++j)cells.push_back(w.pulses[i].mark);}unsigned count=six?21:14;if(!valid||cells.size()!=(six?44:28))continue;std::uint32_t raw=0;unsigned k=0;for(unsigned b=0;b<count;++b){unsigned width=six&&b==4?2:1;bool a=cells[k],z=cells[k+width];if(a==z){valid=false;break;}for(unsigned j=0;j<width;++j)if(cells[k+j]!=a||cells[k+width+j]!=z)valid=false;raw=(raw<<1)|(six?a:!a);k+=2*width;}if(!valid)continue;if(six){if((raw>>17)!=8)continue;return Decoded{Rc6,std::uint16_t((raw>>8)&255),std::uint16_t(raw&255),raw&0xfffff,20,false};}if(!(raw&(1u<<13)))continue;return Decoded{Rc5,std::uint16_t((raw>>6)&31),std::uint16_t((raw&63)|((raw&(1u<<12))?0:64)),raw&0x1fff,13,false};}
 return {};
}
struct Optics {double distance=0,range=1400,current=.0095,intensity=1,sensitivity=1,ambient=0,tolerance=.12;unsigned hz=38000,minimumCycles=6;double minimumStrength=.15;bool lineOfSight=true;};
inline double strength(const Optics& o){return std::max(0.,o.current/.0095*o.intensity*o.sensitivity/(1+std::pow(o.distance/std::max(1.,o.range/2),2))-o.ambient);}
inline std::optional<Wave> demodulate(const Wave& w,const Optics& o){if(!o.lineOfSight||o.distance>o.range||w.hz<1000||w.hz>100000||std::abs(double(w.hz)-o.hz)>o.hz*o.tolerance||w.duty<.1||w.duty>.6||strength(o)<o.minimumStrength||w.pulses.size()>256)return {};for(auto p:w.pulses)if(!p.us||p.us>20000||(p.mark&&double(p.us)*w.hz/1000000<o.minimumCycles))return {};return w;}
}
