#include "thinkercad/modules.hpp"
#include "thinkercad/circuit.hpp"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <stdexcept>
using namespace thinkercad;
void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
int main(){
    for(const auto& [id,d]:ModuleRegistry::definitions()){ModuleRegistry::validate(d);CircuitDocument circuit;const auto part=circuit.addPart(PartKind::Module,100,100);circuit.part(part).model=id;circuit.validate();require(CircuitDocument::pins(circuit.part(part))==d.pins,"module pins");Esp8266Board board;const auto state=circuit.solve(board);for(const auto& pin:CircuitDocument::pins(circuit.part(part)))require(state.voltage.contains({part,pin}),"module pin voltage entry");}
    for(const auto& id:BoardProfile::catalog()){const auto profile=BoardProfile::get(id);Esp8266Board board;board.configure(id);CircuitDocument doc;doc.part("board").model=id;if(id=="attiny85"){auto supply=doc.addPart(PartKind::PowerSupply,0,0);doc.part(supply).value=5;doc.connect({supply,"+"},{"board","VCC"});doc.connect({supply,"-"},{"board","GND"});}doc.validate();for(int pin:profile.gpio){require(profile.pinNumber(profile.pinName(pin))==pin,"pin-name round trip");const auto pins=CircuitDocument::pins(doc.part("board"));require(std::find(pins.begin(),pins.end(),profile.pinName(pin))!=pins.end(),"board pin appears in document");(void)board.pin(pin);}const auto state=doc.solve(board);if(!state.errors.empty())throw std::runtime_error(id+": "+state.errors.front());require(std::abs(*state.voltage.at({"board",id=="attiny85"?"VCC":"3V3"})-(id=="attiny85"?5:3.3))<1e-6,"board supply");}
    for(const auto& id:BoardProfile::catalog()){
        CircuitDocument doc;doc.part("board").model=id;Esp8266Board board;board.configure(id);const auto profile=board.profile();
        const auto names=CircuitDocument::pins(doc.part("board"));const auto power=profile.additionalPowerPins();
        for(const auto& name:power){require(std::count(names.begin(),names.end(),name)==1,"power terminal exists exactly once");require(profile.pinNumber(name)==-1&&!profile.supportsPwm(profile.pinNumber(name)),"power terminal is not a GPIO or PWM pin");}
        if(id=="attiny85"||id=="microbit"){require(power.empty()&&profile.usbPowerPin().empty(),"do not invent 5 V terminals on low-voltage or bare boards");continue;}
        auto load=doc.addPart(PartKind::Resistor,0,0);doc.part(load).value=1000;
        const auto usb=profile.usbPowerPin();doc.connect({"board",usb},{load,"1"});doc.connect({"board","GND"},{load,"2"});
        auto state=doc.solve(board);require(state.errors.empty(),"5 V rail can drive a load");require(std::abs(*state.voltage.at({"board",usb})-5)<1e-8,"USB power terminal supplies 5 V");require(std::abs(*state.branchCurrent.at(load)-.005)<1e-8,"5 V terminal delivers actual load current");
        if(id=="arduino-uno"||id=="arduino-nano"){
            require(!state.voltage.at({"board","VIN"}),"Arduino VIN is an undriven input, not a fake power output");
            const auto supply=doc.addPart(PartKind::Battery9V,0,0);doc.connect({supply,"+"},{"board","VIN"});doc.connect({supply,"-"},{"board","GND"});state=doc.solve(board);
            require(state.errors.empty()&&std::abs(*state.voltage.at({"board","VIN"})-9)<1e-8,"Arduino VIN accepts an external 9 V source without fighting the USB rail");
            require(std::abs(*state.voltage.at({"board","5V"})-5)<1e-8,"VIN and regulated 5 V are separate nets");
        }
        if(id=="rp2040"){
            require(std::abs(*state.voltage.at({"board","VSYS"})-4.7)<.001,"Pico USB reaches VSYS through a Schottky diode");
            const auto supply=doc.addPart(PartKind::PowerSupply,0,0);doc.part(supply).value=5.2;doc.connect({supply,"+"},{"board","VSYS"});doc.connect({supply,"-"},{"board","GND"});state=doc.solve(board);
            require(state.errors.empty()&&std::abs(*state.voltage.at({"board","VSYS"})-5.2)<1e-8,"Pico VSYS accepts an external supply without backfeeding VBUS");
        }
        if(!profile.arduino()){
            doc.connect({"board",usb},{"board",profile.pinName(profile.gpio.front())});const auto bad=doc.solve(board);
            require(std::any_of(bad.errors.begin(),bad.errors.end(),[](const auto& error){return error.find("Boardpin")!=std::string::npos;}),"5 V supply ports do not make 3.3 V GPIO inputs tolerant of 5 V");doc.removeWire(doc.wires().back().id);
        }
        doc.connect({"board",usb},{"board","GND"});require(!doc.solve(board).errors.empty(),"USB power short circuit is detected");
    }
    require(BoardProfile::get("rp2040").pinName(26)=="GP26","Pico pin labels");require(BoardProfile::get("stm32f103").pinName(45)=="PC13","STM32 pin labels");
    auto local=ModuleRegistry::get("bme280");local.id="local-sensor";local.builtin=false;local.implementation="registers";local.registers={{0,42}};ModuleRegistry::install(local);require(ModuleRegistry::get(local.id).registers.at(0)==42,"install local registers");bool rejected=false;try{ModuleRegistry::install(local);}catch(const std::invalid_argument&){rejected=true;}require(rejected,"local collision must be rejected");ModuleRegistry::removeLocal(local.id);ModuleRegistry::removeLocal("bme280");require(ModuleRegistry::get("bme280").builtin,"builtins cannot be removed");
    auto invalid=local;invalid.pins.push_back("VCC");rejected=false;try{ModuleRegistry::validate(invalid);}catch(const std::invalid_argument&){rejected=true;}require(rejected,"duplicate pin rejected");
    std::cout<<"Module descriptors, board profiles and extension validation passed\n";
}
