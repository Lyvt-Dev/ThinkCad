#pragma once
#include "moduledevices.hpp"
namespace thinkercad {
// Native-library bridge for timing-sensitive one-wire devices. State belongs to
// physical parts; masters only own ROM selection and byte-stream cursors.
class SensorBus {
public:
    void reset(){thermometers_.clear();masters_.clear();dht_.clear();eeprom_.clear();}
    void synchronizePower(const CircuitDocument&,const CircuitState&);
    int oneWire(const CircuitDocument&,const CircuitState&,const std::string& board,int pin,
                int operation,int value,Microseconds now,bool emulation);
    int dht(const CircuitDocument&,const CircuitState&,const std::string& board,int pin,
            int index,Microseconds now);
private:
    struct Thermometer {
        std::array<std::uint8_t,9> scratch{0x50,0x05,75,70,0x7f,0xff,0x0c,0x10,0};
        std::array<std::uint8_t,3> saved{75,70,0x7f};
        Microseconds ready=0;
        double temperature=85;
        bool pending=false;
    };
    struct Master {std::vector<std::string> selected;std::vector<std::uint8_t> rom;int command=0,cursor=0,searchBit=0,searchPhase=0;};
    struct Dht {std::array<std::uint8_t,5> bytes{};Microseconds sampled=0;bool valid=false;};
    std::map<std::string,Thermometer> thermometers_;
    std::map<Endpoint,Master> masters_;
    std::map<std::string,Dht> dht_;
    std::map<std::string,std::array<std::uint8_t,3>> eeprom_;
    static std::array<std::uint8_t,8> rom(const std::string& id);
    static std::vector<const CircuitPart*> connected(const CircuitDocument&,const CircuitState&,
            const std::string& board,int pin,const std::string& model,const std::string& data);
};
}
