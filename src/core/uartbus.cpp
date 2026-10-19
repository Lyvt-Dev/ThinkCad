#include "thinkercad/uartbus.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
namespace thinkercad {
std::vector<int> UartBus::supportedPorts(const BoardProfile& profile){
    if(profile.id=="attiny85")return {};if(profile.arduino())return {0};if(profile.id=="rp2040")return {1,2};if(profile.id=="esp8266"||profile.id=="esp32-c3"||profile.id=="esp32-c6"||profile.id=="esp32-s2")return {0,1};return {0,1,2};
}
std::pair<int,int> UartBus::defaultPins(const BoardProfile& p,int port){
    const auto ports=supportedPorts(p);if(std::find(ports.begin(),ports.end(),port)==ports.end())throw std::invalid_argument("UART is unavailable on this board");
    if(p.id=="microbit")return {1,0};if(p.arduino())return {0,1};if(p.id=="esp8266")return port==0?std::pair{3,1}:std::pair{-1,2};
    if(p.id=="rp2040")return port==2?std::pair{5,4}:std::pair{1,0};
    if(p.id=="stm32f103")return port==2?std::pair{3,2}:port==1?std::pair{27,26}:std::pair{10,9};
    if(p.id=="esp32-c3")return port==0?std::pair{20,21}:std::pair{18,19};
    if(p.id=="esp32-c6")return port==0?std::pair{17,16}:std::pair{5,4};
    if(p.id=="esp32-s3"||p.id=="esp32-s2")return port==0?std::pair{44,43}:port==1?std::pair{18,17}:std::pair{16,15};
    return port==0?std::pair{3,1}:port==1?std::pair{16,17}:std::pair{26,27};
}
UartBus::Port& UartBus::require(const std::string& board,int port){const auto at=ports_.find({board,port});if(at==ports_.end())throw std::invalid_argument("UART has not been initialized");return at->second;}
void UartBus::begin(const CircuitDocument& doc,const std::string& board,int number,const Config& config){
    const auto profile=BoardProfile::get(doc.part(board).model);const auto allowed=supportedPorts(profile);if(std::find(allowed.begin(),allowed.end(),number)==allowed.end()||config.baud<300||config.baud>2000000||config.dataBits<5||config.dataBits>8||config.stopBits<1||config.stopBits>2||int(config.parity)<0||int(config.parity)>2||config.rx==config.tx)throw std::invalid_argument("Invalid UART configuration");
    auto valid=[&](int pin,bool output){if(pin<0)return pin==-1;return std::find(profile.gpio.begin(),profile.gpio.end(),pin)!=profile.gpio.end()&&(!output||std::find(profile.inputOnly.begin(),profile.inputOnly.end(),pin)==profile.inputOnly.end());};
    if(!valid(config.rx,false)||!valid(config.tx,true)||(config.rx<0&&config.tx<0)||(profile.id=="esp8266"&&number==1&&config.rx>=0))throw std::invalid_argument("Invalid UART RX/TX pins");
    for(const auto& [key,port]:ports_)if(key.first==board&&key.second!=number)for(int pin:{config.rx,config.tx})if(pin>=0&&(port.config.rx==pin||port.config.tx==pin))throw std::invalid_argument("GPIO is already assigned to another UART");
    if(ports_.size()>=384&&!ports_.contains({board,number}))throw std::invalid_argument("UART port limit exceeded");Port fresh;fresh.config=config;ports_[{board,number}]=std::move(fresh);
}
void UartBus::end(const std::string& board,int number){ports_.erase({board,number});}
void UartBus::write(const CircuitDocument& doc,const CircuitState& state,const std::string& board,int number,const std::vector<std::uint8_t>& bytes,Microseconds now,bool emulation,const std::map<std::string,const Esp8266Board*>& boards){
    synchronizeDevices(doc,state,now);
    if(bytes.size()>512)throw std::invalid_argument("UART write exceeds 512 bytes");auto& source=require(board,number);if(source.config.tx<0)throw std::invalid_argument("UART is receive-only");const auto profile=BoardProfile::get(doc.part(board).model);const Endpoint tx{board,profile.pinName(source.config.tx)};
    const unsigned bits=1+source.config.dataBits+source.config.stopBits+(source.config.parity!=Parity::None);const Microseconds duration=emulation?(bits*1000000ull+source.config.baud-1)/source.config.baud:0;auto start=emulation?std::max(now,source.nextWrite):now;if(duration&&bytes.size()>(std::numeric_limits<Microseconds>::max()-start)/duration)throw std::overflow_error("UART clock overflow");if(start>now&&start-now>10000000)throw std::invalid_argument("UART transmit queue exceeds ten seconds");
    const auto v=state.voltage.find(tx),g=state.voltage.find({board,"GND"});const bool driven=v!=state.voltage.end()&&g!=state.voltage.end()&&v->second&&g->second&&*v->second-*g->second>=.7*profile.voltage;
    struct Sink {Port* port;bool valid;};std::vector<Sink> sinks;
    for(auto& [key,receiver]:ports_){if(receiver.config.rx<0)continue;const auto targetProfile=BoardProfile::get(doc.part(key.first).model);const Endpoint endpoint{key.first,targetProfile.pinName(receiver.config.rx)};bool direct=doc.connected(tx,endpoint)&&doc.connected({board,"GND"},{key.first,"GND"});bool translated=!direct&&wirePath(doc,boards,board,source.config.tx,endpoint);if(!direct&&!translated)continue;
        const bool valid=(translated||(driven&&profile.voltage>=.7*targetProfile.voltage&&profile.voltage<=targetProfile.voltage+.3))&&std::abs(double(source.config.baud)/receiver.config.baud-1)<=.025&&source.config.dataBits==receiver.config.dataBits&&source.config.parity==receiver.config.parity&&source.config.stopBits>=receiver.config.stopBits;sinks.push_back({&receiver,valid});
    }
    for(auto byte:bytes){const auto ready=start+duration;for(const auto& sink:sinks){auto& receiver=*sink.port;if(!sink.valid){++receiver.errors.framing;continue;}
            Byte incoming{start,ready,std::uint8_t(byte&((1u<<source.config.dataBits)-1)),{board,number},true};if(emulation)for(auto& pending:receiver.rx)if(pending.source!=incoming.source&&pending.ready>start&&pending.start<ready){if(pending.valid)++receiver.errors.collision;pending.valid=false;incoming.valid=false;}if(!incoming.valid)++receiver.errors.collision;
            if(receiver.rx.size()>=256){++receiver.errors.overrun;continue;}const auto position=std::upper_bound(receiver.rx.begin(),receiver.rx.end(),ready,[](auto at,const auto& pending){return at<pending.ready;});receiver.rx.insert(position,std::move(incoming));
        }start=ready;}
    source.nextWrite=start;receiveSensors(doc,state,board,number,bytes,start,boards);if(transmitted)transmitted(board,source.config.tx,source.config.baud,bytes,start);
}
void UartBus::discardInvalid(Port& port,Microseconds now){while(!port.rx.empty()&&port.rx.front().ready<=now&&!port.rx.front().valid)port.rx.pop_front();}
unsigned UartBus::available(const std::string& board,int number,Microseconds now){auto& port=require(board,number);discardInvalid(port,now);return unsigned(std::count_if(port.rx.begin(),port.rx.end(),[&](const auto& byte){return byte.ready<=now&&byte.valid;}));}
int UartBus::read(const std::string& board,int number,Microseconds now,bool peek){auto& port=require(board,number);discardInvalid(port,now);if(port.rx.empty()||port.rx.front().ready>now)return -1;const int value=port.rx.front().value;if(!peek)port.rx.pop_front();return value;}
UartBus::Errors UartBus::errors(const std::string& board,int number)const{const auto at=ports_.find({board,number});return at==ports_.end()?Errors{}:at->second.errors;}
UartBus::Config UartBus::config(const std::string& board,int number)const{const auto at=ports_.find({board,number});if(at==ports_.end())throw std::invalid_argument("UART has not been initialized");return at->second.config;}
}

bool thinkercad::UartBus::wirePath(const CircuitDocument& doc,const std::map<std::string,const Esp8266Board*>& boards,const std::string& board,int tx,const Endpoint& receiver){
    auto it=boards.find(board);if(it==boards.end()||!it->second)return false;auto high=*it->second,low=high;high.pinMode(tx,PinMode::Output);low.pinMode(tx,PinMode::Output);high.digitalWrite(tx,true);low.digitalWrite(tx,false);auto sources=boards;sources[board]=&high;const auto on=doc.solve(sources);sources[board]=&low;const auto off=doc.solve(sources);return on.errors.empty()&&off.errors.empty()&&on.level(receiver)==Level::High&&off.level(receiver)==Level::Low;
}
