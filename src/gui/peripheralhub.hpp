#pragma once
#include "thinkercad/circuit.hpp"
#include <QByteArray>
#include <deque>
#include "../../sdk/irwaveform.hpp"
// High-level device adapters. Physical supply and bus wiring are checked on each operation.
// This is packet/NEC/tag emulation, not RF propagation or payment-card emulation.
class PeripheralHub {
public:
    using Answer=std::function<void(int)>;
    void reset(){devices_.clear();pressed_.clear();remoteNext_.clear();tagMemory_.clear();targets_.clear();traces_.clear();now_=0;}
    void inputs(const thinkercad::CircuitDocument&,std::optional<thinkercad::Microseconds> now={});
    bool handle(const std::string&,thinkercad::Esp8266Board&,const thinkercad::CircuitDocument&,const thinkercad::CircuitState&,thinkercad::Microseconds,const QByteArray&,const Answer&);
    struct IrTrace {std::string source,receiver,status;thinkercad::Microseconds start=0;thinkir::Wave emitted,received;double strength=0;std::optional<thinkir::Decoded> decoded;};
    std::map<std::string,thinkercad::DigitalWaveform> irSignals()const;
    const std::deque<IrTrace>& irTraces()const{return traces_;}
    std::optional<bool> irLevel(const std::string& board,int pin,thinkercad::Microseconds now)const;
private:
    struct IrFrame {int address,command,flags;thinkir::Decoded decoded;thinkir::Wave wave;thinkercad::Microseconds ready=0;};
    std::deque<IrTrace> traces_;thinkercad::Microseconds now_=0;
    void transmit(const thinkercad::CircuitDocument&,const thinkercad::CircuitState&,const std::vector<std::pair<std::string,double>>&,const thinkir::Wave&,thinkercad::Microseconds);

    struct Packet {QByteArray bytes;thinkercad::Microseconds ready=0;int pipe=0;};
    struct Device {std::string board,id,type;std::vector<int> pins;std::vector<std::string> emitters;std::optional<thinkir::Decoded> lastIr;thinkercad::Microseconds lastIrAt=0;int channel=76,rate=1000,crc=2;bool listening=false,ack=true;std::map<int,QByteArray> pipes;QByteArray destination;std::deque<Packet> packets;std::deque<IrFrame> infrared;bool held=false;std::string card;std::uint64_t cardGeneration=0;};
    std::map<std::pair<std::string,int>,Device> devices_;
    std::map<std::string,bool> pressed_;std::map<std::string,thinkercad::Microseconds> remoteNext_;
    std::map<std::string,QByteArray> tagMemory_;
    struct Target {std::pair<std::string,int> owner;QByteArray uid,memory;bool writable=true;std::uint64_t generation=0;};
    std::map<std::string,Target> targets_; // One RF role per physical PN532.
    std::uint64_t targetGeneration_=0;
    void infrared(const thinkercad::CircuitDocument&,const std::string&,int,int,int flags=0);
};
