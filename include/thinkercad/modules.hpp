#pragma once
#include <map>
#include <string>
#include <vector>
namespace thinkercad {
struct ModuleParameter {
    std::string name,label,unit;
    double initial=0,min=0,max=0;
    int registerAddress=-1,bytes=1;
    double scale=1;
    bool littleEndian=false,signedValue=false;
};
struct ModuleDefinition {
    std::string id,label,bus,implementation,power="VCC",ground="GND";
    std::vector<std::string> pins;
    std::map<std::string,std::pair<double,double>> pinPositions;
    std::vector<ModuleParameter> parameters;
    double width=18,height=24,minVoltage=3,maxVoltage=3.6;
    int address=0,spiMode=0;
    unsigned maxClock=400000;
    std::map<unsigned,unsigned> registers;
    std::vector<unsigned> readOnly;
    bool builtin=false;
};
// Descriptor-only extension point. No shared libraries or scripts are executed.
class ModuleRegistry {
public:
    static const std::map<std::string,ModuleDefinition>& definitions();
    static const ModuleDefinition& get(const std::string& id);
    static void validate(const ModuleDefinition& definition);
    static void install(ModuleDefinition definition);
    static void removeLocal(const std::string& id);
};
}
