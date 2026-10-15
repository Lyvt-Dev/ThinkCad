#pragma once
#include "PeripheralProtocol.h"
class RCSwitch {
public:
    void enableTransmit(int pin){pinMode(pin,OUTPUT);digitalWrite(pin,LOW);if(!thinker::peripheralRequest("ASK_TX_BEGIN",tx_,std::to_string(pin)))throw std::runtime_error("ASK transmitter wiring/power invalid");txReady_=true;}
    void enableReceive(int pin){pinMode(pin,INPUT);if(!thinker::peripheralRequest("ASK_RX_BEGIN",rx_,std::to_string(pin)))throw std::runtime_error("ASK receiver wiring/power invalid");rxReady_=true;}
    void setProtocol(int protocol){if(protocol!=1)throw std::invalid_argument("ASK adapter supports rc-switch protocol 1");}void setPulseLength(int length){if(length!=350)throw std::invalid_argument("ASK adapter supports default 350 us pulses");}void setRepeatTransmit(int n){if(n<1||n>20)throw std::invalid_argument("ASK repeat count 1..20");repeats_=n;}
    void send(unsigned long value,unsigned int bits){if(!txReady_||bits<1||bits>32||(bits<32&&(value>>bits)))throw std::invalid_argument("ASK value does not fit 1..32 bits");std::ostringstream hex;hex<<std::hex<<value;thinker::peripheralRequest("ASK_SEND",tx_,std::to_string(bits)+" 1 "+hex.str());delayMicroseconds((bits*4+31)*350*repeats_);}
    bool available(){if(!rxReady_)return false;if(bits_)return true;bits_=thinker::peripheralRequest("ASK_READ",rx_);if(bits_){auto lo=thinker::peripheralRead(),hi=thinker::peripheralRead();value_=uint32_t(lo)|(uint32_t(hi)<<16);}return bits_!=0;}
    unsigned long getReceivedValue()const{return value_;}unsigned int getReceivedBitlength()const{return bits_;}unsigned int getReceivedProtocol()const{return bits_?1:0;}void resetAvailable(){bits_=0;value_=0;}
private:int tx_=thinker::peripheralToken(),rx_=thinker::peripheralToken(),bits_=0,repeats_=10;uint32_t value_=0;bool txReady_=false,rxReady_=false;
};
