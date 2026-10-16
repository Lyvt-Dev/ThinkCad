#include "thinkercad/interrupts.hpp"
#include <iostream>
#include <stdexcept>
using namespace thinkercad;
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
int main(){
    CircuitDocument doc;Esp8266Board board;board.pinMode(5,PinMode::Output);board.pinMode(4,PinMode::Input);doc.connect({"board","GPIO5"},{"board","GPIO4"});InterruptController irq;
    irq.attach(doc,doc.solve(board),"board",4,InterruptController::Mode::Rising);require(irq.take("board").empty(),"attaching does not invent a rising edge");board.digitalWrite(5,true);irq.sample(doc,doc.solve(board));require(irq.take("board")==std::vector<int>{4}&&irq.take("board").empty(),"rising edge is delivered once");board.digitalWrite(5,false);irq.sample(doc,doc.solve(board));require(irq.take("board").empty(),"falling edge does not trigger rising handler");
    irq.attach(doc,doc.solve(board),"board",4,InterruptController::Mode::Change);for(bool value:{true,false,true,false}){board.digitalWrite(5,value);irq.sample(doc,doc.solve(board));}require(irq.take("board")==std::vector<int>{4},"pending edges coalesce to one flag per pin");irq.detach("board",4);require(irq.empty()&&irq.take("board").empty(),"detach clears pending interrupt state");
    irq.attach(doc,doc.solve(board),"board",4,InterruptController::Mode::Low);require(irq.take("board")==std::vector<int>{4}&&irq.take("board")==std::vector<int>{4},"low-level interrupt remains asserted until line rises");board.digitalWrite(5,true);irq.sample(doc,doc.solve(board));require(irq.take("board").empty(),"level release stops delivery");bool rejected=false;try{irq.attach(doc,doc.solve(board),"board",16,InterruptController::Mode::Rising);}catch(const std::invalid_argument&){rejected=true;}require(rejected,"ESP8266 GPIO16 has no external interrupt");
    CircuitDocument floating;irq.reset();irq.attach(floating,floating.solve(board),"board",4,InterruptController::Mode::Rising);CircuitState state;state.voltage[{"board","GPIO4"}]=3.3;irq.sample(floating,state);require(irq.take("board").empty(),"floating-to-known transition is not fabricated as an edge");
    std::cout<<"GPIO interrupt edges, level behavior, pending flags, detach and unsupported pins passed\n";
}
