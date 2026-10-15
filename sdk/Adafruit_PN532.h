#pragma once
#include "PeripheralProtocol.h"
#include "Wire.h"
constexpr uint8_t PN532_MIFARE_ISO14443A=0;
class Adafruit_PN532 {
public:
    Adafruit_PN532(uint8_t irq,uint8_t reset,TwoWire* wire=&Wire):irq_(irq),reset_(reset),wire_(wire){}
    bool begin(){if(!wire_->ready())wire_->begin();pinMode(irq_,INPUT);pinMode(reset_,OUTPUT);digitalWrite(reset_,HIGH);ready_=thinker::peripheralRequest("NFC_BEGIN",token_,std::to_string(wire_->sda())+" "+std::to_string(wire_->scl())+" "+std::to_string(irq_)+" "+std::to_string(reset_));return ready_;}
    uint32_t getFirmwareVersion(){return ready_?0x32010607:0;}bool SAMConfig(){return ready_;}
    bool readPassiveTargetID(uint8_t baud,uint8_t* uid,uint8_t* length,uint16_t timeout=100){if(baud!=PN532_MIFARE_ISO14443A||!uid||!length)throw std::invalid_argument("PN532 adapter supports ISO14443A test tags");if(!ready_)return false;auto started=millis();do{int n=thinker::peripheralRequest("NFC_POLL",token_);if(n){if(n!=4&&n!=7)throw std::runtime_error("Invalid NFC UID");*length=n;for(int i=0;i<n;++i)uid[i]=thinker::peripheralRead();return true;}delay(10);}while(!timeout||millis()-started<timeout);*length=0;return false;}
    bool ntag2xx_ReadPage(uint8_t page,uint8_t* data){if(!data||!ready_)return false;int n=thinker::peripheralRequest("NFC_PAGE",token_,std::to_string(page));for(int i=0;i<n;++i){auto b=thinker::peripheralRead();if(i<4)data[i]=b;}return n==4;}
private:int token_=thinker::peripheralToken();uint8_t irq_,reset_;TwoWire* wire_;bool ready_=false;
};
