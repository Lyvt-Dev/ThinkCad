#pragma once
#include "circuit.hpp"
#include <deque>
namespace thinkercad {
class UartBus {
public:
    enum class Parity {None,Even,Odd};
    struct Config {unsigned baud=115200;int rx=-1,tx=-1,dataBits=8,stopBits=1;Parity parity=Parity::None;};
    struct Errors {unsigned framing=0,overrun=0,collision=0;};
    std::function<void(const std::string&,int,unsigned,const std::vector<std::uint8_t>&,Microseconds)> transmitted;
    static bool wirePath(const CircuitDocument&,const std::map<std::string,const Esp8266Board*>&,const std::string& board,int tx,const Endpoint& receiver);
    void reset(){ports_.clear();sensors_.clear();modules_.clear();}
    void synchronizeDevices(const CircuitDocument&,const CircuitState&,Microseconds now=0);
    void begin(const CircuitDocument&,const std::string& board,int port,const Config&);
    void end(const std::string& board,int port);
    void write(const CircuitDocument&,const CircuitState&,const std::string& board,int port,
               const std::vector<std::uint8_t>& bytes,Microseconds now,bool emulation,const std::map<std::string,const Esp8266Board*>& boards={});
    unsigned available(const std::string& board,int port,Microseconds now);
    int read(const std::string& board,int port,Microseconds now,bool peek=false);
    Errors errors(const std::string& board,int port)const;
    Config config(const std::string& board,int port)const;
    bool active(const std::string& board,int port)const{return ports_.contains({board,port});}
    static std::vector<int> supportedPorts(const BoardProfile&);
    static std::pair<int,int> defaultPins(const BoardProfile&,int port); // RX, TX
private:
    using Key=std::pair<std::string,int>;
    struct Byte {Microseconds start=0,ready=0;std::uint8_t value=0;Key source;bool valid=true;};
    struct Port {Config config;std::deque<Byte> rx;Microseconds nextWrite=0;Errors errors;};
    struct ModulePort {std::string model;std::vector<std::uint8_t> input;bool echo=true;Microseconds nextWrite=0;};
    std::map<std::string,ModulePort> modules_;
    std::map<Key,Port> ports_;
    struct Co2Sensor {std::vector<std::uint8_t> command;Microseconds nextResponse=0;};
    std::map<std::string,Co2Sensor> sensors_;
    void receiveSensors(const CircuitDocument&,const CircuitState&,const std::string& board,int port,
                        const std::vector<std::uint8_t>&,Microseconds,const std::map<std::string,const Esp8266Board*>&);
    void writeModules(const CircuitDocument&,const CircuitState&,const std::string&,int,const std::vector<std::uint8_t>&,Microseconds,bool,const std::map<std::string,const Esp8266Board*>&);
    Port& require(const std::string&,int);
    static void discardInvalid(Port&,Microseconds);
};
}
