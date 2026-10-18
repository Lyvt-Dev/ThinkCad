#pragma once
#include "nettopology.hpp"
#include "uartbus.hpp"
#include "bustraffic.hpp"
namespace thinkercad {
enum class Severity { Info, Warning, Error };
struct DiagnosticIssue {
    std::string rule, explanation, solution;
    Severity severity = Severity::Warning;
    Endpoint target;
    std::string wire;
};
class DiagnosticEngine {
  public:
    static std::vector<DiagnosticIssue> analyze(const CircuitDocument &, const NetTopology &,
                                                const CircuitState &,
                                                const std::map<std::string, const Esp8266Board *> &,
                                                const UartBus *uart = nullptr,
                                                const BusTraffic *traffic = nullptr);
};
} // namespace thinkercad
