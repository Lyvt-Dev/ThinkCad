#include "thinkercad/sensorbus.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace thinkercad {
std::array<std::uint8_t,8> SensorBus::rom(const std::string& id){
    std::uint64_t hash=14695981039346656037ull;for(unsigned char c:id){hash^=c;hash*=1099511628211ull;}
    std::array<std::uint8_t,8> result{0x28};for(int i=1;i<7;++i){result[i]=std::uint8_t(hash);hash>>=8;}result[7]=ModuleDevices::crc8({result.begin(),result.begin()+7});return result;
}
std::vector<const CircuitPart*> SensorBus::connected(const CircuitDocument& doc,const CircuitState& state,const std::string& board,int pin,const std::string& model,const std::string& data){
    const auto profile=BoardProfile::get(doc.part(board).model);if(profile.pinNumber(profile.pinName(pin))<0)throw std::invalid_argument("Invalid sensor GPIO");std::vector<const CircuitPart*> result;
    for(const auto& part:doc.parts())if(part.kind==PartKind::Module&&part.model==model){const auto& d=ModuleRegistry::get(model);auto voltage=[&](const std::string& name)->std::optional<double>{auto it=state.voltage.find({part.id,name});return it==state.voltage.end()?std::nullopt:it->second;};const auto v=voltage(d.power),g=voltage(d.ground),level=voltage(data);if(!v||!g||!level||*v-*g<d.minVoltage||*v-*g>d.maxVoltage||*level-*g<.7*(*v-*g))continue;if(doc.connected({part.id,d.ground},{board,"GND"})&&doc.connected({part.id,data},{board,profile.pinName(pin)}))result.push_back(&part);}
    std::sort(result.begin(),result.end(),[](auto* a,auto* b){return rom(a->id)<rom(b->id);});return result;
}
int SensorBus::oneWire(const CircuitDocument& doc,const CircuitState& circuit,const std::string& board,int pin,int op,int value,Microseconds now,bool emulation){
    if(op<0||op>7||value<0||value>8191)throw std::invalid_argument("Invalid 1-Wire request");
    auto parts=connected(doc,circuit,board,pin,"ds18b20","DQ");auto& master=masters_[{board,std::to_string(pin)}];
    for(const auto* part:parts){auto& s=thermometers_[part->id];if(s.pending&&now>=s.ready){const int resolution=9+((s.scratch[4]>>5)&3);int raw=int(std::llround(s.temperature*16));raw&=~((1<<(12-resolution))-1);s.scratch[0]=std::uint8_t(raw);s.scratch[1]=std::uint8_t(raw>>8);s.pending=false;}s.scratch[8]=ModuleDevices::crc8({s.scratch.begin(),s.scratch.begin()+8});}
    if(op==0){master=Master{};return !parts.empty();}
    if(op==4){master=Master{};return 0;}
    if(op==5){const bool alarm=value&4096;const unsigned index=(value&4095)>>3,byte=value&7;if(alarm)std::erase_if(parts,[&](const auto* part){const auto& s=thermometers_.at(part->id);const auto raw=std::int16_t((s.scratch[1]<<8)|s.scratch[0]);return raw/16<=std::int8_t(s.scratch[2])&&raw/16>=std::int8_t(s.scratch[3]);});return index<parts.size()?rom(parts[index]->id)[byte]:-1;}
    auto selected=[&](const auto* part){return std::find(master.selected.begin(),master.selected.end(),part->id)!=master.selected.end();};
    if(op==3){for(const auto* part:parts)if(selected(part)&&thermometers_.at(part->id).pending)return 0;return 1;}
    if(op==7)return 0;
    if(op==2){int out=255;for(const auto* part:parts)if(selected(part)){const auto& state=thermometers_.at(part->id);if(master.command==0xbe&&master.cursor<9)out&=state.scratch[master.cursor];else if(master.command==0x33&&master.cursor<8)out&=rom(part->id)[master.cursor];}if(master.cursor<16)++master.cursor;return out;}
    if(value>255)throw std::invalid_argument("Invalid 1-Wire byte");
    if(master.command==0x55&&master.rom.size()<8){master.rom.push_back(std::uint8_t(value));if(master.rom.size()==8){for(const auto* part:parts){const auto address=rom(part->id);if(std::equal(address.begin(),address.end(),master.rom.begin()))master.selected.push_back(part->id);}master.command=0;}return 0;}
    if(master.command==0x4e&&master.cursor<3){for(const auto* part:parts)if(selected(part)){auto& s=thermometers_.at(part->id);s.scratch[2+master.cursor]=master.cursor==2?std::uint8_t((value&0x60)|0x1f):std::uint8_t(value);}if(++master.cursor==3)master.command=0;return 0;}
    master.command=value;master.cursor=0;
    if(value==0x55){master.selected.clear();master.rom.clear();}
    else if(value==0xcc||value==0x33){master.selected.clear();for(const auto* part:parts)master.selected.push_back(part->id);if(value==0xcc)master.command=0;}
    else for(const auto* part:parts)if(selected(part)){auto& s=thermometers_.at(part->id);if(value==0x44){s.temperature=parameter(*part,"temperature",25);s.pending=true;s.ready=now+(emulation?(750000ull>>(3-((s.scratch[4]>>5)&3))):0);}else if(value==0x48)std::copy_n(s.scratch.begin()+2,3,s.saved.begin());else if(value==0xb8)std::copy(s.saved.begin(),s.saved.end(),s.scratch.begin()+2);}
    return 0;
}
int SensorBus::dht(const CircuitDocument& doc,const CircuitState& state,const std::string& board,int pin,int index,Microseconds now){
    if(index<0||index>4)throw std::invalid_argument("Invalid DHT byte");const auto parts=connected(doc,state,board,pin,"dht22","DATA");if(parts.size()!=1)return -1;const auto& part=*parts.front();auto& sample=dht_[part.id];if(!sample.valid||now-sample.sampled>=2000000){const int humidity=int(std::llround(parameter(part,"humidity",50)*10));const double t=parameter(part,"temperature",25);const int temperature=int(std::llround(std::abs(t)*10))|(t<0?0x8000:0);sample.bytes={std::uint8_t(humidity>>8),std::uint8_t(humidity),std::uint8_t(temperature>>8),std::uint8_t(temperature),0};sample.bytes[4]=std::uint8_t(sample.bytes[0]+sample.bytes[1]+sample.bytes[2]+sample.bytes[3]);sample.sampled=now;sample.valid=true;}return sample.bytes[index];
}
}
