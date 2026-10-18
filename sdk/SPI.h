#pragma once
#include "Arduino.h"
#define SPI_HAS_TRANSACTION 1
using BitOrder=int;
constexpr int MSBFIRST=1,LSBFIRST=0,SPI_MODE0=0,SPI_MODE1=1,SPI_MODE2=2,SPI_MODE3=3;
class SPISettings {
public:
    explicit SPISettings(unsigned long clock=1000000,int order=MSBFIRST,int mode=SPI_MODE0):clock_(clock),order_(order),mode_(mode){if(!clock||clock>80000000||(order!=MSBFIRST&&order!=LSBFIRST)||mode<SPI_MODE0||mode>SPI_MODE3)throw std::invalid_argument("Invalid SPI settings");}
    unsigned long clock()const{return clock_;}int order()const{return order_;}int mode()const{return mode_;}
private:unsigned long clock_;int order_,mode_;
};
class SPIClass {
public:
    void begin(int sck=-1,int miso=-1,int mosi=-1,int ss=-1){
#if defined(THINKERCAD_GENERIC_BOARD)
        const int defaults[]={THINKERCAD_BOARD_SPI_SCK,THINKERCAD_BOARD_SPI_MISO,THINKERCAD_BOARD_SPI_MOSI,THINKERCAD_BOARD_SPI_CS};
#elif defined(THINKERCAD_ARDUINO_UNO)||defined(THINKERCAD_ARDUINO_NANO)
        const int defaults[]={13,12,11,10};
#elif defined(THINKERCAD_ESP32_S3)
        const int defaults[]={12,13,11,10};
#elif defined(THINKERCAD_ESP32)
        const int defaults[]={18,19,23,5};
#else
        const int defaults[]={14,12,13,15};
#endif
        int values[]={sck,miso,mosi,ss};for(int i=0;i<4;++i){pins_[i]=values[i]<0?defaults[i]:values[i];thinker::validatePin(pins_[i]);}if(pins_[0]==pins_[1]||pins_[0]==pins_[2]||pins_[1]==pins_[2])throw std::invalid_argument("SPI data/clock pins must differ");pinMode(pins_[0],OUTPUT);pinMode(pins_[1],INPUT);pinMode(pins_[2],OUTPUT);pinMode(pins_[3],OUTPUT);digitalWrite(pins_[3],HIGH);ready_=true;
    }
    bool ready()const{return ready_;}int sck()const{return pins_[0];}int miso()const{return pins_[1];}int mosi()const{return pins_[2];}
    void end(){ready_=false;}
    void beginTransaction(const SPISettings& settings){if(!ready_)begin();settings_=settings;}
    void endTransaction(){}
    void setFrequency(unsigned long frequency){settings_=SPISettings(frequency,settings_.order(),settings_.mode());}
    void setBitOrder(int order){settings_=SPISettings(settings_.clock(),order,settings_.mode());}
    void setDataMode(int mode){settings_=SPISettings(settings_.clock(),settings_.order(),mode);}
    uint8_t transfer(uint8_t value){
        if(!ready_)throw std::runtime_error("Call SPI.begin() first");const int result=thinker::request("SPI_BYTE "+std::to_string(pins_[0])+" "+std::to_string(pins_[1])+" "+std::to_string(pins_[2])+" "+std::to_string(settings_.clock())+" "+std::to_string(settings_.mode())+" "+std::to_string(settings_.order())+" "+std::to_string(value)+" 0");if(result<0||result>255)throw std::runtime_error("Invalid SPI host byte");
#ifdef THINKERCAD_EMULATION
        thinker::advance((8000000ull+settings_.clock()-1)/settings_.clock());
#endif
        return uint8_t(result);
    }
    uint16_t transfer16(uint16_t value){if(settings_.order()==LSBFIRST){const auto lo=transfer(uint8_t(value));return uint16_t(lo|(uint16_t(transfer(uint8_t(value>>8)))<<8));}const auto hi=transfer(uint8_t(value>>8));return uint16_t((uint16_t(hi)<<8)|transfer(uint8_t(value)));}
    void transfer(void* buffer,std::size_t length){if((!buffer&&length)||length>4096)throw std::invalid_argument("Invalid SPI transfer buffer");auto* bytes=static_cast<uint8_t*>(buffer);for(std::size_t i=0;i<length;++i)bytes[i]=transfer(bytes[i]);}
private:int pins_[4]{};bool ready_=false;SPISettings settings_;
};
inline SPIClass SPI;
