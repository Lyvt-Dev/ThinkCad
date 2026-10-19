#pragma once
#include "electronics.hpp"
namespace thinkercad {
void registerActuators(std::map<std::string, ModuleDefinition> &);
bool actuatorBranches(const CircuitPart &, const std::function<std::optional<double>(const std::string &)> &,
                      const DeviceRegisters *, std::vector<ElectricalBranch> &);
void observeActuators(const CircuitDocument &, const CircuitState &, DeviceRegisters &, Microseconds);
void describeActuators(const CircuitDocument &, const DeviceRegisters &, CircuitState &);
} // namespace thinkercad
