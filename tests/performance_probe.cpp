#include "thinkercad/circuit.hpp"
#include "thinkercad/moduledevices.hpp"
#include "thinkercad/bustraffic.hpp"
#include <chrono>
#include <iostream>
#include <stdexcept>
using namespace thinkercad;
using Clock=std::chrono::steady_clock;
int main(){
    CircuitDocument document;document.part("board").model="esp32";Esp8266Board board;board.configure("esp32");std::vector<int> outputs;
    for(int pin:board.profile().gpio)if(std::find(board.profile().inputOnly.begin(),board.profile().inputOnly.end(),pin)==board.profile().inputOnly.end()&&outputs.size()<16){outputs.push_back(pin);board.pinMode(pin,PinMode::Output);const auto resistor=document.addPart(PartKind::Resistor,300,outputs.size()*40),led=document.addPart(PartKind::Led,500,outputs.size()*40);document.part(resistor).value=330;document.connect({"board",board.profile().pinName(pin)},{resistor,"1"});document.connect({resistor,"2"},{led,"A"});document.connect({led,"K"},{"board","GND"});}
    const auto begin=Clock::now();for(int step=0;step<500;++step){for(int pin:outputs)board.digitalWrite(pin,step%2);const auto state=document.solve(board);if(!state.errors.empty())throw std::runtime_error(state.errors.front());}
    const auto solved=Clock::now();ModuleDevices devices;CircuitPart sensor;sensor.id="sensor";sensor.kind=PartKind::Module;sensor.model="bme280";for(int step=0;step<100000;++step){devices.i2c(sensor,{0xd0},-1,step);if(devices.i2c(sensor,{},1,step).at(0)!=0x60)throw std::runtime_error("Invalid benchmark register reply");}
    const auto transferred=Clock::now();BusTraffic traffic;traffic.enabled=true;for(int step=0;step<100000;++step)traffic.append({Microseconds(step),"board","UART","1","TX queued",115200,std::vector<std::uint8_t>(512,0x55),{}});const auto captured=Clock::now();if(traffic.entries().size()!=1024||traffic.entries().back().txBytes!=512)throw std::runtime_error("Unbounded benchmark capture");
    auto ms=[](auto a,auto b){return std::chrono::duration<double,std::milli>(b-a).count();};
    std::cout<<"circuit_parts="<<document.parts().size()<<" circuit_wires="<<document.wires().size()<<" solves=500 solve_ms="<<ms(begin,solved)<<"\nregister_reads=100000 register_ms="<<ms(solved,transferred)<<"\ntrace_packets=100000 trace_ms="<<ms(transferred,captured)<<" retained="<<traffic.entries().size()<<" overwritten="<<traffic.overwritten()<<" payload_capacity="<<traffic.entries().back().tx.capacity()<<"\n";
}
