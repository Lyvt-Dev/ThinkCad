#pragma once
#include "thinkercad/circuit.hpp"
#include <algorithm>

namespace BoardPins {
// Keep existing project contact coordinates stable when power terminals are added.
inline std::vector<std::string> legacy(const thinkercad::CircuitPart& part){
    auto pins=thinkercad::CircuitDocument::pins(part);
    const auto added=thinkercad::BoardProfile::get(part.model).additionalPowerPins();
    std::erase_if(pins,[&](const auto& pin){return std::find(added.begin(),added.end(),pin)!=added.end();});
    return pins;
}
}
