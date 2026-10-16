#include "thinkercad/circuit.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace thinkercad;
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
int main(){
    CircuitDocument doc;doc.part("board").model="arduino-uno";Esp8266Board board;board.configure("arduino-uno");const auto bridge=doc.addPart(PartKind::Module,250,0);doc.part(bridge).model="l298n";const auto battery=doc.addPart(PartKind::Battery9V,500,0),motor=doc.addPart(PartKind::DcMotor,500,200);doc.part(motor).value=10;
    doc.connect({"board","5V"},{bridge,"VCC"});doc.connect({"board","GND"},{bridge,"GND"});doc.connect({battery,"-"},{"board","GND"});doc.connect({battery,"+"},{bridge,"VS"});doc.connect({bridge,"OUT1"},{motor,"+"});doc.connect({bridge,"OUT2"},{motor,"-"});
    for(const auto& [pin,name]:std::vector<std::pair<int,std::string>>{{3,"ENA"},{4,"IN1"},{5,"IN2"},{6,"ENB"},{7,"IN3"},{8,"IN4"}}){board.pinMode(pin,PinMode::Output);doc.connect({"board","D"+std::to_string(pin)},{bridge,name});}
    auto state=doc.solve(board);require(!state.voltage.at({bridge,"OUT1"})&&!state.voltage.at({bridge,"OUT2"}),"disabled bridge outputs are high impedance");
    board.digitalWrite(3,true);board.digitalWrite(4,true);state=doc.solve(board);require(state.errors.empty(),"forward network converges");const double current=*state.branchCurrent.at(motor);require(std::abs(current-7./10.6)<1e-6,"bridge includes saturation voltage and output resistance");
    board.digitalWrite(4,false);board.digitalWrite(5,true);state=doc.solve(board);require(state.errors.empty()&&*state.branchCurrent.at(motor)<-.65,"direction reverses current");board.digitalWrite(4,true);state=doc.solve(board);require(std::abs(*state.branchCurrent.at(motor))<1e-8&&*state.voltage.at({bridge,"OUT1"})>7,"equal inputs actively brake at common high rail");
    board.digitalWrite(3,false);state=doc.solve(board);require(!state.voltage.at({bridge,"OUT1"}),"disable releases output");board.digitalWrite(3,true);board.pinMode(4,PinMode::Input);state=doc.solve(board);require(std::abs(*state.branchCurrent.at(motor))<1e-8,"floating control releases driver instead of asserting a logic value");
    board.pinMode(4,PinMode::Output);board.digitalWrite(4,true);board.digitalWrite(5,false);TransientSimulation transient;const auto dynamic=transient.advance(doc,{{"board",&board}},10000);require(dynamic.errors.empty()&&*dynamic.branchCurrent.at(motor)>0,"bridge drives existing motor transient model");
    std::cout<<"L298N truth table, voltage loss, load current and motor transient checks passed\n";
}
