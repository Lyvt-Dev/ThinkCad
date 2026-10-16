#pragma once
#include "simulation.hpp"
#include <optional>
#include <set>
#include <tuple>
namespace thinkercad {
enum class PartKind { Esp8266, Oled, Led, Resistor, Button, AnalogSource, Potentiometer, Photoresistor, Thermistor, SlideSwitch, TiltSwitch, ReedSwitch, Diode, Battery3V, Battery9V, PowerSupply, RGBLed, SevenSegment, Voltmeter, Breadboard30, PiezoBuzzer, Speaker, TemperatureSensor, BatteryAA, ToggleSwitch, Capacitor, Inductor, NpnTransistor, PnpTransistor, NMosfet, PMosfet, OpAmp, Ground, FunctionGenerator, Oscilloscope, LogicAnalyzer, CurrentProbe, Junction, NetLabel, CharacterLcd, I2cCharacterLcd, AndGate, NandGate, OrGate, NorGate, XorGate, XnorGate, NotGate, ServoMotor, DcMotor, InfraredLed, InfraredReceiver, InfraredRemote, NfcReader, NfcTestCard, RadioTransmitter, RadioReceiver, RadioTransceiver, Note, Module };
struct ComponentSpec {PartKind kind;std::string label,unit;double initial,min,max;bool adjustable;};
const std::vector<ComponentSpec>& componentCatalog();
const ComponentSpec& componentSpec(PartKind kind);
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
    std::string color="#fb586b"; // Visual LED tint; electrical model still uses Vf=2 V.
    std::string model;
    int rotation=0; // Clockwise quarter-turns in degrees, around the component center.
    std::map<std::string,double> parameters;
    std::string netName;
};
double parameter(const CircuitPart& part,const std::string& name,double fallback);
struct WirePoint { double x=0,y=0; };
struct CircuitWire { std::string id; Endpoint a,b; std::string color="#0d9488"; std::vector<WirePoint> points; };
struct SoundSignal {
    std::string id;
    double frequency=0, amplitude=0; // Normalized AC peak, including electrical attenuation.
};
struct CircuitIssue {std::string part,message;};
struct CircuitState {
    std::map<Endpoint,std::optional<double>> voltage;
    std::map<Endpoint,double> logicVoltage;
    std::map<std::string,double> ledCurrent;
    std::map<std::string,std::optional<double>> branchCurrent;
    std::map<std::string,double> partValues;
    std::map<std::string,std::vector<std::uint32_t>> pixels;
    std::vector<std::string> errors;
    std::vector<CircuitIssue> warnings;
    Level level(const Endpoint& endpoint) const;
};
// Serializable, GUI-independent circuit document. Wires join nodes electrically.
struct UltrasonicState {bool high=false;Microseconds trigger=0,rise=0,fall=0,ready=0;};
struct ReactiveState {std::map<std::string,double> capacitorVoltage,inductorCurrent,motorCurrent,motorSpeed,servoRise,servoPulse;std::map<std::string,bool> servoLevel;std::map<std::string,UltrasonicState> ultrasonic;};
class TransientSimulation;
class CircuitDocument {
public:
    CircuitDocument();
    std::string boardId()const;
    void setActiveBoard(const std::string& id);
    const std::vector<CircuitPart>& parts() const { return parts_; }
    const std::vector<CircuitWire>& wires() const { return wires_; }
    CircuitPart& part(const std::string& id);
    const CircuitPart& part(const std::string& id) const;
    static std::vector<std::string> pins(PartKind kind);
    static std::vector<std::string> pins(const CircuitPart& part);
    static int displayHeight(const CircuitPart& part){return part.model=="ssd1306-128x32"?32:64;}
    std::string addPart(PartKind kind,double x,double y);
    void insertPart(CircuitPart part);
    void removePart(const std::string& id);
    std::string connect(Endpoint a,Endpoint b,std::string color="#0d9488");
    void removeWire(const std::string& id);
    bool connected(const Endpoint& a,const Endpoint& b) const;
    void clearWires();
    void validate() const;
    CircuitState solve(const Esp8266Board& board) const;
    CircuitState solve(const std::map<std::string,const Esp8266Board*>& boards) const;
    std::vector<SoundSignal> soundSignals(const std::map<std::string,const Esp8266Board*>& boards,std::vector<std::string>* errors=nullptr) const;
    bool oledReady(const std::string& id,int sda,int scl,const CircuitState& state,const std::string& boardId={}) const;
private:
    friend class TransientSimulation;
    CircuitState solveStep(const std::map<std::string,const Esp8266Board*>& boards,const ReactiveState* memory,double dt,double time)const;
    std::string activeBoard_;
    std::vector<CircuitPart> parts_;
    std::vector<CircuitWire> wires_;
    std::uint64_t nextPart_=1,nextWire_=1;
    void validateEndpoint(const Endpoint& endpoint) const;
};
class TransientSimulation {
public:
    using Boards=std::map<std::string,const Esp8266Board*>;
    void reset();
    Microseconds now()const{return now_;}
    void setMaximumStep(Microseconds step);
    CircuitState evaluate(const CircuitDocument& document,const Boards& boards);
    std::pair<Microseconds,Microseconds> pulse(const CircuitDocument&,const CircuitState&,const Boards&,
        const std::string& board,int pin,bool high,Microseconds timeout)const;
    CircuitState advance(const CircuitDocument& document,const Boards& boards,Microseconds target,
                         const std::function<void(Microseconds,const CircuitState&)>& sample={});
private:
    ReactiveState memory_;
    Microseconds now_=0,maximumStep_=100;
    void observeUltrasonic(const CircuitDocument&,const CircuitState&);
};
}
