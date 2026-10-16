#pragma once
#include "Arduino.h"
#include "PeripheralProtocol.h"
#include "Stream.h"
#include <vector>

// Arduino-compatible master API. Transactions are executed by the simulated bus.
class TwoWire : public Stream {
public:
    explicit TwoWire(int bus=0) {if(bus!=0)throw std::invalid_argument("Only the primary I2C controller is modeled");}
    using Print::write;
    bool begin(){return begin(SDA,SCL);}
    bool begin(int sda,int scl) {
        thinker::validatePin(sda);thinker::validatePin(scl);
        if(sda==scl)throw std::invalid_argument("SDA and SCL must differ");
        sda_=sda;scl_=scl;initialized_=true;transmitting_=false;tx_.clear();rx_.clear();cursor_=0;
        std::cout<<"I2C "<<sda<<' '<<scl<<std::endl;return true;
    }
    void begin(uint8_t) {throw std::runtime_error("I2C slave-mode sketches are not modeled; use master mode with a supported device");}
    void end(){if(initialized_)std::cout<<"I2C_END"<<std::endl;initialized_=false;transmitting_=false;tx_.clear();rx_.clear();cursor_=0;}
    void setClock(unsigned long hz){if(!hz||hz>400000)throw std::invalid_argument("I2C supports 1..400000 Hz");hz_=hz;}
    unsigned long getClock()const{return hz_;}
    unsigned long clock()const{return hz_;}
    int sda()const{return sda_;}int scl()const{return scl_;}bool ready()const{return initialized_;}
    void beginTransmission(int address){requireReady();validateAddress(address);address_=address;tx_.clear();overflow_=false;transmitting_=true;clearWriteError();}
    std::size_t write(uint8_t value)override{
        if(!transmitting_||tx_.size()>=bufferLength){setWriteError();overflow_=true;return 0;}
        tx_.push_back(value);return 1;
    }
    std::size_t write(const uint8_t* data,std::size_t size)override{
        if(!data&&size){setWriteError();return 0;}
        std::size_t n=0;while(n<size&&write(data[n]))++n;return n;
    }
    int availableForWrite()override{return transmitting_?static_cast<int>(bufferLength-tx_.size()):0;}
    uint8_t endTransmission(bool sendStop=true){
        requireReady();if(!transmitting_)return 4;transmitting_=false;if(overflow_)return 1;
        const auto hex=tx_.empty()?"-":thinker::peripheralHex(tx_.data(),tx_.size());
        int status=thinker::request("I2C_WRITE "+std::to_string(address_)+" "+std::to_string(sendStop)+" "+std::to_string(hz_)+" "+hex);
        if(status<0||status>5)throw std::runtime_error("Invalid I2C host status");
        transferTime(tx_.size());return static_cast<uint8_t>(status);
    }
    std::size_t requestFrom(int address,int quantity,bool sendStop=true){
        requireReady();validateAddress(address);if(quantity<0)throw std::invalid_argument("Negative I2C read length");
        const auto count=std::min<std::size_t>(quantity,bufferLength);rx_.clear();cursor_=0;
        int n=thinker::request("I2C_READ "+std::to_string(address)+" "+std::to_string(count)+" "+std::to_string(sendStop)+" "+std::to_string(hz_));
        if(n<0||static_cast<std::size_t>(n)>count)throw std::runtime_error("Invalid I2C host read length");
        for(int i=0;i<n;++i){const int value=thinker::peripheralRead();if(value<0||value>255)throw std::runtime_error("Invalid I2C byte");rx_.push_back(static_cast<uint8_t>(value));}
        transferTime(n);return rx_.size();
    }
    std::size_t requestFrom(uint8_t address,uint8_t count,uint32_t internalAddress,uint8_t internalSize,bool stop=true){
        beginTransmission(address);internalSize=std::min<uint8_t>(internalSize,3);
        for(int i=internalSize-1;i>=0;--i)write(static_cast<uint8_t>(internalAddress>>(8*i)));
        if(endTransmission(false)){rx_.clear();cursor_=0;return 0;}return requestFrom(address,count,stop);
    }
    int available()override{return static_cast<int>(rx_.size()-cursor_);}
    int read()override{return available()?rx_[cursor_++]:-1;}
    int peek()override{return available()?rx_[cursor_]:-1;}
    // AVR Wire::flush does not transmit or discard pending I2C data.
    void flush()override{}
    void onReceive(void (*)(int)){throw std::runtime_error("I2C slave callbacks are not modeled");}
    void onRequest(void (*)()){throw std::runtime_error("I2C slave callbacks are not modeled");}
#if defined(THINKERCAD_ARDUINO_UNO)||defined(THINKERCAD_ARDUINO_NANO)
    static constexpr std::size_t bufferLength=32;
#else
    static constexpr std::size_t bufferLength=128;
#endif
protected:
    uint64_t streamClock()const override{return thinker::clockUs;}
    void streamWait(uint64_t us)override{thinker::advance(us);}
private:
    int sda_=SDA,scl_=SCL,address_=0;unsigned long hz_=100000;
    bool initialized_=false,transmitting_=false,overflow_=false;
    std::vector<uint8_t> tx_,rx_;std::size_t cursor_=0;
    void requireReady()const{if(!initialized_)throw std::runtime_error("Call Wire.begin() first");}
    static void validateAddress(int address){if(address<0||address>127)throw std::invalid_argument("I2C requires a 7-bit address");}
    void transferTime(std::size_t bytes){thinker::advance(((bytes+1)*9ull*1000000+hz_-1)/hz_);}
};
inline TwoWire Wire;
