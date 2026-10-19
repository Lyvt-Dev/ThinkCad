#pragma once
#include "circuit.hpp"
namespace thinkercad {
std::array<std::uint8_t, 256> bme680Calibration();
void sampleBme680(const CircuitPart &, std::array<std::uint8_t, 256> &, Microseconds now);
Microseconds bme680Duration(const std::array<std::uint8_t, 256> &);
} // namespace thinkercad
