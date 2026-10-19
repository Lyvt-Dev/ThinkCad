#include "thinkercad/pixelbus.hpp"
#include "thinkercad/modules.hpp"
#include <algorithm>
#include <set>
#include <stdexcept>
namespace thinkercad {
bool PixelBus::powered(const CircuitPart& p,const CircuitState& state){auto v=state.voltage.find({p.id,"VCC"}),g=state.voltage.find({p.id,"GND"});return v!=state.voltage.end()&&g!=state.voltage.end()&&v->second&&g->second&&*v->second-*g->second>=3.5&&*v->second-*g->second<=5.3;}
bool PixelBus::transmit(const CircuitDocument& doc,const CircuitState& state,const std::string& board,int pin,unsigned clock,const std::vector<std::uint8_t>& bytes,Microseconds now){
    if(clock!=800000||bytes.size()>3072||bytes.empty()||bytes.size()%3)throw std::invalid_argument("Invalid WS2812B frame or clock");
    const auto profile=BoardProfile::get(doc.part(board).model);const Endpoint source{board,profile.pinName(pin)};if(profile.pinNumber(source.pin)<0)throw std::invalid_argument("Invalid NeoPixel pin");
    bool delivered=false;std::set<std::string> visited;
    auto deliver=[&](auto&& self,const Endpoint& from,std::size_t offset)->void{
        for(const auto& part:doc.parts())if(part.kind==PartKind::Module&&ModuleRegistry::get(part.model).implementation=="ws2812"&&!visited.contains(part.id)&&doc.connected(from,{part.id,"DIN"})&&doc.connected({part.id,"GND"},{board,"GND"})&&powered(part,state)){
            const double supply=*state.voltage.at({part.id,"VCC"})-*state.voltage.at({part.id,"GND"});if(from==source&&(profile.voltage<.7*supply||profile.voltage>supply+.5))continue;
            visited.insert(part.id);if(latched_.contains(part.id)&&now-latched_.at(part.id)<50)continue;
            const auto count=std::size_t(std::clamp(neoPixelCount(part),1,256));auto& frame=frames_[part.id];frame.resize(count,0);const auto available=std::min(count,(bytes.size()-std::min(offset,bytes.size()))/3);for(std::size_t i=0;i<available;++i){const auto at=offset+i*3;frame[i]=(std::uint32_t(bytes[at+1])<<16)|(std::uint32_t(bytes[at])<<8)|bytes[at+2];}latched_[part.id]=now;delivered=true;if(offset+count*3<bytes.size())self(self,{part.id,"DOUT"},offset+count*3);
        }
    };
    deliver(deliver,source,0);return delivered;
}
void PixelBus::apply(const CircuitDocument& doc,CircuitState& state){
    std::set<std::string> present;for(const auto& part:doc.parts())if(part.kind==PartKind::Module&&ModuleRegistry::get(part.model).implementation=="ws2812"){present.insert(part.id);state.pixels.erase(part.id);if(!powered(part,state)){frames_.erase(part.id);latched_.erase(part.id);continue;}const auto frame=frames_.find(part.id);if(frame!=frames_.end())state.pixels[part.id]=frame->second;}
    std::erase_if(frames_,[&](const auto& entry){return !present.contains(entry.first);});std::erase_if(latched_,[&](const auto& entry){return !present.contains(entry.first);});
}
}
