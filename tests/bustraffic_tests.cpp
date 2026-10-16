#include "thinkercad/bustraffic.hpp"
#include <iostream>
#include <stdexcept>
using namespace thinkercad;
void require(bool condition){if(!condition)throw std::runtime_error("Bus trace invariant failed");}
int main(){
    BusTraffic trace;trace.append({1,"board","I2C","sensor","ACK",100000,{1,2},{3}});require(trace.entries().empty());
    trace.enabled=true;trace.setCapacity(128);for(unsigned i=0;i<1000;++i)trace.append({i,"board","SPI","sensor","OK",1000000,{std::uint8_t(i)},{0x42}});require(trace.entries().size()==128);require(trace.entries().front().time==872);require(trace.overwritten()==872);
    trace.append({1001,"board","UART","1","TX queued",9600,std::vector<std::uint8_t>(512,0xaa),{}});require(trace.entries().back().tx.size()==128);require(trace.entries().back().txBytes==512);const auto revision=trace.revision();trace.clear();require(trace.entries().empty()&&trace.overwritten()==0&&trace.revision()>revision);trace.enabled=false;trace.append({});require(trace.entries().empty());std::cout<<"Disabled capture, bounded retention and payload lengths passed\n";
}
