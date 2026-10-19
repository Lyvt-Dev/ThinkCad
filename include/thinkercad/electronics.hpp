#pragma once
#include "modules.hpp"
#include "circuit.hpp"
#include <functional>
#include <array>
namespace thinkercad {
void registerElectronics(std::map<std::string,ModuleDefinition>&);
using DeviceRegisters=std::map<std::string,std::array<std::uint64_t,16>>;
struct ElectricalBranch {std::string a,b;double resistance,offset=0;bool operator==(const ElectricalBranch&)const=default;};
std::vector<ElectricalBranch> electronicBranches(const CircuitPart&,const std::function<std::optional<double>(const std::string&)>&,const DeviceRegisters*,double time,const std::map<std::string,AnalogDeviceState>* analog=nullptr);
void advanceAnalogDevices(const CircuitDocument&,const CircuitState&,ReactiveState&,Microseconds now);
void observeElectronics(const CircuitDocument&,const CircuitState&,DeviceRegisters&,Microseconds now=0);
}
