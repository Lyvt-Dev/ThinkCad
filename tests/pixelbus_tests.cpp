#include "thinkercad/pixelbus.hpp"
#include <iostream>
#include <stdexcept>
using namespace thinkercad;
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
int main(){
    CircuitDocument doc;doc.part("board").model="arduino-uno";Esp8266Board board;board.configure("arduino-uno");board.pinMode(2,PinMode::Output);PixelBus bus;
    auto add=[&](int count){auto id=doc.addPart(PartKind::Module,300,0);doc.part(id).model="ws2812";doc.part(id).parameters["pixels"]=count;doc.connect({"board","5V"},{id,"VCC"});doc.connect({"board","GND"},{id,"GND"});return id;};
    const auto first=add(2),second=add(1);doc.connect({"board","D2"},{first,"DIN"});doc.connect({first,"DOUT"},{second,"DIN"});std::vector<std::uint8_t> frame{0,255,0,255,0,0,0,0,255};auto state=doc.solve(board);require(bus.transmit(doc,state,"board",2,800000,frame,1000),"wired chain receives a frame");bus.apply(doc,state);require(state.pixels.at(first)==std::vector<std::uint32_t>({0xff0000,0x00ff00})&&state.pixels.at(second)==std::vector<std::uint32_t>({0x0000ff}),"GRB data and DOUT chain offsets");
    const std::vector<std::uint8_t> off(frame.size(),0);require(!bus.transmit(doc,state,"board",2,800000,off,1020),"reset interval required before new latch");require(bus.transmit(doc,state,"board",2,800000,off,2000),"frame after reset interval accepted");bus.apply(doc,state);require(state.pixels.at(first)[0]==0,"new colors reach physical part state");
    doc.part("board").model="esp8266";board.configure("esp8266");doc.clearWires();doc.connect({"board","3V3"},{first,"VCC"});doc.connect({"board","GND"},{first,"GND"});doc.connect({"board","GPIO2"},{first,"DIN"});state=doc.solve(board);require(!bus.transmit(doc,state,"board",2,800000,frame,3000),"undervoltage strip does not light");bus.apply(doc,state);require(!state.pixels.contains(first),"power loss clears retained colors");
    CircuitDocument esp;const auto strip=esp.addPart(PartKind::Module,200,0);esp.part(strip).model="ws2812";const auto supply=esp.addPart(PartKind::PowerSupply,400,0);esp.part(supply).value=5;esp.connect({supply,"+"},{strip,"VCC"});esp.connect({supply,"-"},{"board","GND"});esp.connect({strip,"GND"},{"board","GND"});esp.connect({strip,"DIN"},{"board","GPIO2"});require(!bus.transmit(esp,esp.solve(board),"board",2,800000,frame,4000),"5 V pixels require valid input logic level");
    std::cout<<"NeoPixel GRB frames, chains, latch interval, power and logic level checks passed\n";
}
