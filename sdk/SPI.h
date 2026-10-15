#pragma once
#include "Arduino.h"
constexpr int MSBFIRST=1,LSBFIRST=0,SPI_MODE0=0,SPI_MODE1=1,SPI_MODE2=2,SPI_MODE3=3;
class SPISettings {public:SPISettings(unsigned long=1000000,int=MSBFIRST,int=SPI_MODE0){}};
class SPIClass {
public:
    void begin(int sck=-1,int miso=-1,int mosi=-1,int ss=-1){
#if defined(THINKERCAD_ARDUINO_UNO)||defined(THINKERCAD_ARDUINO_NANO)
        const int defaults[]={13,12,11,10};
#elif defined(THINKERCAD_ESP32_S3)
        const int defaults[]={12,13,11,10};
#elif defined(THINKERCAD_ESP32)
        const int defaults[]={18,19,23,5};
#else
        const int defaults[]={14,12,13,15};
#endif
        int values[]={sck,miso,mosi,ss};for(int i=0;i<4;++i){pins_[i]=values[i]<0?defaults[i]:values[i];thinker::validatePin(pins_[i]);}ready_=true;
    }
    bool ready()const{return ready_;}int sck()const{return pins_[0];}int miso()const{return pins_[1];}int mosi()const{return pins_[2];}
    void end(){ready_=false;}
    void beginTransaction(const SPISettings&){}void endTransaction(){}
    uint8_t transfer(uint8_t){throw std::runtime_error("Raw SPI register transfer is not modeled; use the RF24 adapter");}
private:int pins_[4]{};bool ready_=false;
};
inline SPIClass SPI;
