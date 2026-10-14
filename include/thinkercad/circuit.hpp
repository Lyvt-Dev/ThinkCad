#pragma once
#include "simulation.hpp"
#include <optional>
#include <set>
#include <tuple>
namespace thinkercad {
enum class PartKind { Esp8266, Oled, Led, Resistor, Button, AnalogSource };
struct Endpoint {
    std::string part, pin;
    bool operator==(const Endpoint&) const = default;
    bool operator<(const Endpoint& other) const { return std::tie(part,pin)<std::tie(other.part,other.pin); }
};
struct CircuitPart {
    std::string id;
    PartKind kind;
    double x=0,y=0;
    double value=220; // Ohms for a resistor, volts for an analog source.
    bool pressed=false;
};
struct CircuitWire { std::string id; Endpoint a,b; std::string color="#0d9488"; };
struct CircuitState {
    std::map<Endpoint,std::optional<double>> voltage;
    std::map<std::string,double> ledCurrent;
    std::vector<std::string> errors;
    Level level(const Endpoint& endpoint) const;
};
// Serializable, GUI-independent circuit document. Wires join nodes electrically.
class CircuitDocument {
public:
    CircuitDocument();
    const std::vector<CircuitPart>& parts() const { return parts_; }
    const std::vector<CircuitWire>& wires() const { return wires_; }
    CircuitPart& part(const std::string& id);
    const CircuitPart& part(const std::string& id) const;
    static std::vector<std::string> pins(PartKind kind);
    std::string addPart(PartKind kind,double x,double y);
    void insertPart(CircuitPart part);
    void removePart(const std::string& id);
    std::string connect(Endpoint a,Endpoint b,std::string color="#0d9488");
    void removeWire(const std::string& id);
    bool connected(const Endpoint& a,const Endpoint& b) const;
    void clearWires();
    void validate() const;
    CircuitState solve(const Esp8266Board& board) const;
    bool oledReady(const std::string& id,int sda,int scl,const CircuitState& state) const;
private:
    std::vector<CircuitPart> parts_;
    std::vector<CircuitWire> wires_;
    std::uint64_t nextPart_=1,nextWire_=1;
    void validateEndpoint(const Endpoint& endpoint) const;
};
}
