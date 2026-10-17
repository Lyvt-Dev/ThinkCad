#include "thinkercad/interrupts.hpp"
#include <algorithm>
#include <stdexcept>
namespace thinkercad {
void InterruptController::attach(const CircuitDocument& doc,const CircuitState& state,const std::string& board,int pin,Mode mode){
    const auto profile=BoardProfile::get(doc.part(board).model);if(std::find(profile.gpio.begin(),profile.gpio.end(),pin)==profile.gpio.end()||(profile.arduino()&&pin!=2&&pin!=3)||(profile.id=="esp8266"&&pin==16)||int(mode)<0||int(mode)>3)throw std::invalid_argument("Unavailable GPIO interrupt or mode");
    handlers_[board][pin]={mode,state.level({board,profile.pinName(pin)}),false};
}
void InterruptController::detach(const std::string& board,int pin){const auto at=handlers_.find(board);if(at==handlers_.end())return;at->second.erase(pin);if(at->second.empty())handlers_.erase(at);}
void InterruptController::sample(const CircuitDocument& doc,const CircuitState& state){
    for(auto& [board,handlers]:handlers_){const auto profile=BoardProfile::get(doc.part(board).model);for(auto& [pin,handler]:handlers){const auto level=state.level({board,profile.pinName(pin)});const bool known=(level==Level::High||level==Level::Low)&&(handler.previous==Level::High||handler.previous==Level::Low);if(known&&level!=handler.previous&&(handler.mode==Mode::Change||(handler.mode==Mode::Rising&&level==Level::High)||(handler.mode==Mode::Falling&&level==Level::Low)))handler.pending=true;handler.previous=level;}}
}
std::vector<int> InterruptController::take(const std::string& board){
    std::vector<int> pending;const auto at=handlers_.find(board);if(at==handlers_.end())return pending;for(auto& [pin,handler]:at->second)if(handler.pending||(handler.mode==Mode::Low&&handler.previous==Level::Low)){pending.push_back(pin);handler.pending=false;}return pending;
}
}
