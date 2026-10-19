#pragma once
#include "Wire.h"
class Adafruit_MCP4725 {
    TwoWire* wire_=&Wire;uint8_t address_=0x62;
public:
    bool begin(uint8_t address=0x62,TwoWire* wire=&Wire){if(!wire)return false;wire_=wire;address_=address;if(!wire_->ready())wire_->begin();wire_->beginTransmission(address_);return wire_->endTransmission()==0;}
    bool setVoltage(uint16_t value,bool writeEEPROM,bool=false){if(value>4095)return false;wire_->beginTransmission(address_);wire_->write(uint8_t(writeEEPROM?0x60:0x40));wire_->write(uint8_t(value>>4));wire_->write(uint8_t(value<<4));return wire_->endTransmission()==0;}
};
