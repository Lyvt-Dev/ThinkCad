#pragma once
#include "circuit.hpp"
#include <deque>
namespace thinkercad {
class UartBus {
public:
    enum class Parity {None,Even,Odd};
    struct Config {unsigned baud=115200;int rx=-1,tx=-1,dataBits=8,stopBits=1;Parity parity=Parity::None;};
    struct Errors {unsigned framing=0,overrun=0,collision=0;};
    void reset(){ports_.clear();}
    void begin(const CircuitDocument&,const std::string& board,int port,const Config&);
    void end(const std::string& board,int port);
    void write(const CircuitDocument&,const CircuitState&,const std::string& board,int port,
               const std::vector<std::uint8_t>& bytes,Microseconds now,bool emulation);
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
    std::map<Key,Port> ports_;
    Port& require(const std::string&,int);
    static void discardInvalid(Port&,Microseconds);
};
}
