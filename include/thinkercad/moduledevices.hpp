#pragma once
#include "circuit.hpp"
#include "modules.hpp"
#include <array>
#include <deque>
namespace thinkercad {
// Transaction-level device state, shared by every master using a physical part.
// Registers and conversion clocks are independent of the GUI and native SDK.
class ModuleDevices {
public:
    void reset(){states_.clear();cards_.clear();analog_.clear();}
    void setEmulation(bool value){emulation_=value;}
    void synchronizePower(const CircuitDocument&,const CircuitState&);
    std::vector<std::uint8_t> i2c(const CircuitPart&,const std::vector<std::uint8_t>& data,int count,Microseconds now);
    void select(const CircuitPart&,bool active);
    std::uint8_t spi(const CircuitPart&,std::uint8_t value,Microseconds now);
    void setCard(const std::string& id,std::vector<std::uint8_t> uid){cards_[id]=std::move(uid);}
    void setAnalog(const std::string& id,const std::array<double,8>& volts,double reference);
    std::array<std::uint8_t,256> registers(const CircuitPart&,Microseconds now);
    std::optional<std::array<std::uint8_t,256>> peekRegisters(const std::string& id)const;
    static std::uint8_t crc8(const std::vector<std::uint8_t>& bytes);
private:
    struct State {
        std::array<std::uint8_t,256> reg{};
        std::string model;
        std::deque<std::uint8_t> fifo;
        bool initialized=false,selected=false,command=false,reading=false,pending=false,halted=false;
        std::uint8_t pointer=0;
        int stage=0,adc=0;
        Microseconds ready=0,resetUntil=0,nextSample=0;
    };
    struct Analog{std::array<double,8> volts{};double reference=3.3;};
    std::map<std::string,State> states_;
    std::map<std::string,std::vector<std::uint8_t>> cards_;
    std::map<std::string,Analog> analog_;
    bool emulation_=false;
    State& prepare(const CircuitPart&,Microseconds now);
    void initialize(State&,const ModuleDefinition&);
    void write(State&,const CircuitPart&,std::uint8_t address,std::uint8_t value,Microseconds now);
    std::uint8_t read(State&,const ModuleDefinition&,std::uint8_t address);
    void radio(State&,const CircuitPart&);
    static void sampleBme(State&,const CircuitPart&);
    static void sampleMpu(State&,const CircuitPart&);
};
}
