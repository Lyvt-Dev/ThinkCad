#pragma once
#include "Wire.h"
constexpr uint8_t MPR121_I2CADDR_DEFAULT=0x5a,MPR121_TOUCHSTATUS_L=0,MPR121_ECR=0x5e,MPR121_SOFTRESET=0x80;
class Adafruit_MPR121 {
    TwoWire* wire_=&Wire;uint8_t address_=0x5a;
public:
    bool begin(uint8_t address=MPR121_I2CADDR_DEFAULT,TwoWire* wire=&Wire,uint8_t touch=12,uint8_t release=6,bool=false){if(!wire)return false;wire_=wire;address_=address;if(!wire_->ready())wire_->begin();writeRegister(MPR121_SOFTRESET,0x63);if(readRegister8(0x5d)!=0x24)return false;writeRegister(MPR121_ECR,0);setThresholds(touch,release);writeRegister(MPR121_ECR,0x8c);return true;}
    uint16_t touched(){return readRegister16(0)&0x0fff;}
    uint16_t filteredData(uint8_t electrode){return electrode<12?readRegister16(4+2*electrode)&1023:0;}
    uint16_t baselineData(uint8_t electrode){return electrode<12?uint16_t(readRegister8(0x1e + electrode))<<2:0;}
    void setThresholds(uint8_t touch,uint8_t release){for(int i=0;i<12;++i){writeRegister(0x41+i*2,touch);writeRegister(0x42+i*2,release);}}
    void writeRegister(uint8_t reg,uint8_t value){wire_->beginTransmission(address_);wire_->write(reg);wire_->write(value);wire_->endTransmission();}
    uint8_t readRegister8(uint8_t reg){wire_->beginTransmission(address_);wire_->write(reg);if(wire_->endTransmission(false)||wire_->requestFrom(address_,1)!=1)return 0;return uint8_t(wire_->read());}
    uint16_t readRegister16(uint8_t reg){wire_->beginTransmission(address_);wire_->write(reg);if(wire_->endTransmission(false)||wire_->requestFrom(address_,2)!=2)return 0;auto low=uint8_t(wire_->read());return low|(uint16_t(uint8_t(wire_->read()))<<8);}
};
