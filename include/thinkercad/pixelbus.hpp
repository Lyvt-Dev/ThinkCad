#pragma once
#include "circuit.hpp"
namespace thinkercad {
class PixelBus {
public:
    void reset(){frames_.clear();latched_.clear();}
    bool transmit(const CircuitDocument&,const CircuitState&,const std::string& board,int pin,
                  unsigned clock,const std::vector<std::uint8_t>& bytes,Microseconds now);
    void apply(const CircuitDocument&,CircuitState&);
private:
    std::map<std::string,std::vector<std::uint32_t>> frames_;
    std::map<std::string,Microseconds> latched_;
    static bool powered(const CircuitPart&,const CircuitState&);
};
}
