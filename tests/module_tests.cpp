#include "thinkercad/modules.hpp"
#include "thinkercad/circuit.hpp"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <stdexcept>
using namespace thinkercad;
void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
int main(){
    for(const auto& [id,d]:ModuleRegistry::definitions()){ModuleRegistry::validate(d);CircuitDocument circuit;const auto part=circuit.addPart(PartKind::Module,100,100);circuit.part(part).model=id;circuit.validate();require(CircuitDocument::pins(circuit.part(part))==d.pins,"module pins");Esp8266Board board;const auto state=circuit.solve(board);require(state.voltage.contains({part,d.power}),"module voltage entry");}
    for(const auto& id:BoardProfile::catalog()){const auto profile=BoardProfile::get(id);Esp8266Board board;board.configure(id);CircuitDocument doc;doc.part("board").model=id;doc.validate();for(int pin:profile.gpio){require(profile.pinNumber(profile.pinName(pin))==pin,"pin-name round trip");const auto pins=CircuitDocument::pins(doc.part("board"));require(std::find(pins.begin(),pins.end(),profile.pinName(pin))!=pins.end(),"board pin appears in document");(void)board.pin(pin);}const auto state=doc.solve(board);require(state.errors.empty(),"idle new board");require(std::abs(*state.voltage.at({"board","3V3"})-3.3)<1e-6,"board supply");}
    require(BoardProfile::get("rp2040").pinName(26)=="GP26","Pico pin labels");require(BoardProfile::get("stm32f103").pinName(45)=="PC13","STM32 pin labels");
    auto local=ModuleRegistry::get("bme280");local.id="local-sensor";local.builtin=false;local.implementation="registers";local.registers={{0,42}};ModuleRegistry::install(local);require(ModuleRegistry::get(local.id).registers.at(0)==42,"install local registers");bool rejected=false;try{ModuleRegistry::install(local);}catch(const std::invalid_argument&){rejected=true;}require(rejected,"local collision must be rejected");ModuleRegistry::removeLocal(local.id);ModuleRegistry::removeLocal("bme280");require(ModuleRegistry::get("bme280").builtin,"builtins cannot be removed");
    auto invalid=local;invalid.pins.push_back("VCC");rejected=false;try{ModuleRegistry::validate(invalid);}catch(const std::invalid_argument&){rejected=true;}require(rejected,"duplicate pin rejected");
    std::cout<<"Module descriptors, board profiles and extension validation passed\n";
}
