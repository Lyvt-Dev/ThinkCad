#pragma once
#include "moduledevices.hpp"
namespace thinkercad {
class SpiBus {
public:
    enum class Status {Ok,Unselected,Collision,Clock,Mode,Reference};
    struct Reply {std::uint8_t value=255;Status status=Status::Unselected;std::string id;};
    explicit SpiBus(ModuleDevices& devices):devices_(devices){}
    void refreshSelects(const CircuitDocument&,const CircuitState&);
    Reply transfer(const CircuitDocument&,const CircuitState&,const std::string& board,
                   int sck,int miso,int mosi,unsigned clock,int mode,bool lsb,
                   std::uint8_t value,Microseconds now);
    static const char* description(Status);
    Reply writeBlock(const CircuitDocument&,const CircuitState&,const std::string& board,
                     int sck,int miso,int mosi,unsigned clock,int mode,bool lsb,
                     const std::vector<std::uint8_t>&,Microseconds now);
private:
    ModuleDevices& devices_;
};
}
