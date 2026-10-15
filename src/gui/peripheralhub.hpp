#pragma once
#include "thinkercad/circuit.hpp"
#include <QByteArray>
#include <deque>
// High-level device adapters. Physical supply and bus wiring are checked on each operation.
// This is packet/NEC/tag emulation, not RF propagation or payment-card emulation.
class PeripheralHub {
public:
    using Answer=std::function<void(int)>;
    void reset(){devices_.clear();pressed_.clear();}
    void inputs(const thinkercad::CircuitDocument&);
    bool handle(const std::string&,thinkercad::Esp8266Board&,const thinkercad::CircuitDocument&,const thinkercad::CircuitState&,thinkercad::Microseconds,const QByteArray&,const Answer&);
private:
    struct IrFrame {int address,command,flags;};
    struct Packet {QByteArray bytes;thinkercad::Microseconds ready=0;int pipe=0;};
    struct Device {std::string board,id,type;std::vector<int> pins;int channel=76,rate=1000,crc=2;bool listening=false,ack=true;std::map<int,QByteArray> pipes;QByteArray destination;std::deque<Packet> packets;std::deque<IrFrame> infrared;bool held=false;std::string card;};
    std::map<std::pair<std::string,int>,Device> devices_;
    std::map<std::string,bool> pressed_;
    void infrared(const thinkercad::CircuitDocument&,const std::string&,int,int,int flags=0);
};
