#include "thinkercad/circuit.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace thinkercad;
int checks=0;
void check(bool ok,const char* text){++checks;if(!ok)throw std::runtime_error(text);}
int main(){try{
    Esp8266Board board;CircuitDocument doc;
    auto led=doc.addPart(PartKind::Led,300,0),r=doc.addPart(PartKind::Resistor,250,0);
    doc.connect({"board","GPIO14"},{r,"1"});doc.connect({r,"2"},{led,"A"});doc.connect({led,"K"},{"board","GND"});
    board.pinMode(14,PinMode::Output);board.digitalWrite(14,true);auto state=doc.solve(board);
    check(state.errors.empty(),"valid LED network");check(std::abs(state.ledCurrent.at(led)-1.3/230)<1e-9,"LED and resistor current");
    board.digitalWrite(14,false);check(doc.solve(board).ledCurrent.at(led)<1e-9,"GPIO LOW turns off LED");
    doc.clearWires();doc.connect({"board","3V3"},{led,"A"});doc.connect({led,"K"},{"board","GND"});check(!doc.solve(board).errors.empty(),"missing resistor detects LED overcurrent");
    doc.clearWires();doc.connect({"board","3V3"},{"board","GND"});check(!doc.solve(board).errors.empty(),"power short detected");
    CircuitDocument buttonDoc;auto button=buttonDoc.addPart(PartKind::Button,200,0);buttonDoc.connect({button,"1"},{"board","GPIO12"});buttonDoc.connect({button,"2"},{"board","GND"});board.pinMode(12,PinMode::InputPullup);
    check(buttonDoc.solve(board).level({"board","GPIO12"})==Level::High,"open button with pullup");buttonDoc.part(button).pressed=true;check(buttonDoc.solve(board).level({"board","GPIO12"})==Level::Low,"pressed button grounds GPIO");
    board.pinMode(12,PinMode::Input);buttonDoc.part(button).pressed=false;check(buttonDoc.solve(board).level({"board","GPIO12"})==Level::Floating,"floating pin preserved");
    CircuitDocument oled;auto display=oled.addPart(PartKind::Oled,300,0);oled.connect({display,"VCC"},{"board","3V3"});oled.connect({display,"GND"},{"board","GND"});oled.connect({display,"SDA"},{"board","GPIO4"});oled.connect({display,"SCL"},{"board","GPIO5"});
    check(oled.oledReady(display,4,5,oled.solve(board)),"correct display wiring");check(!oled.oledReady(display,5,4,oled.solve(board)),"swapped I2C pins fail");oled.clearWires();check(!oled.oledReady(display,4,5,oled.solve(board)),"disconnected display cannot run");
    CircuitDocument analog;auto src=analog.addPart(PartKind::AnalogSource,200,0);analog.connect({src,"OUT"},{"board","A0"});analog.connect({src,"GND"},{"board","GND"});check(std::abs(*analog.solve(board).voltage.at({"board","A0"})-0.5)<1e-12,"wired analog source");
    auto count=analog.wires().size();analog.removePart(src);check(count==2&&analog.wires().empty(),"removing component removes attached wires");
    std::cout<<checks<<" circuit checks passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
