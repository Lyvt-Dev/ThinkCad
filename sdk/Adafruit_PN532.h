#pragma once
#include "PeripheralProtocol.h"
#include "Wire.h"
#include <cstring>
constexpr uint8_t PN532_MIFARE_ISO14443A=0;
class Adafruit_PN532 {
public:
    Adafruit_PN532(uint8_t irq,uint8_t reset,TwoWire* wire=&Wire):irq_(irq),reset_(reset),wire_(wire){if(!wire)throw std::invalid_argument("PN532 needs an I2C bus");}
    bool begin(){if(!wire_->ready())wire_->begin();pinMode(irq_,INPUT);pinMode(reset_,OUTPUT);digitalWrite(reset_,HIGH);ready_=thinker::peripheralRequest("NFC_BEGIN",token_,std::to_string(wire_->sda())+" "+std::to_string(wire_->scl())+" "+std::to_string(irq_)+" "+std::to_string(reset_));return ready_;}
    uint32_t getFirmwareVersion(){return ready_?0x32010607:0;}bool SAMConfig(){return ready_;}
    bool readPassiveTargetID(uint8_t baud,uint8_t* uid,uint8_t* length,uint16_t timeout=100){if(baud!=PN532_MIFARE_ISO14443A||!uid||!length)throw std::invalid_argument("PN532 adapter supports ISO14443A test tags");*length=0;if(!ready_)return false;auto started=millis();do{int n=thinker::peripheralRequest("NFC_POLL",token_);if(n){if(n!=4&&n!=7)throw std::runtime_error("Invalid NFC UID");*length=n;for(int i=0;i<n;++i)uid[i]=thinker::peripheralRead();return true;}delay(10);}while(!timeout||millis()-started<timeout);*length=0;return false;}
    bool ntag2xx_ReadPage(uint8_t page,uint8_t* data){if(!data||!ready_)return false;int n=thinker::peripheralRequest("NFC_PAGE",token_,std::to_string(page));for(int i=0;i<n;++i){auto b=thinker::peripheralRead();if(i<4)data[i]=b;}return n==4;}
    bool ntag2xx_WritePage(uint8_t page,const uint8_t* data){if(!data||!ready_||page<4||page>39)return false;return thinker::peripheralRequest("NFC_WRITE_PAGE",token_,std::to_string(page)+" "+thinker::peripheralHex(data,4))!=0;}
    // ThinkCad simulation extensions, not APIs from the upstream Adafruit library.
    // Present a synthetic tag to another powered PN532 in this project.
    bool thinkcadBeginTagEmulation(const uint8_t* uid,uint8_t length,const char* text="ThinkCad emulated tag",bool writable=true){
        if(!ready_||!uid||(length!=4&&length!=7)||!text)return false;
        auto size=std::strlen(text);if(size>96)return false;
        auto encoded=size?thinker::peripheralHex(text,size):std::string("-");
        return thinker::peripheralRequest("NFC_TARGET_BEGIN",token_,thinker::peripheralHex(uid,length)+" "+encoded+" "+(writable?"1":"0"))!=0;
    }
    bool thinkcadStopTagEmulation(){return ready_&&thinker::peripheralRequest("NFC_TARGET_STOP",token_)!=0;}
    bool thinkcadTagEmulationActive(){return ready_&&thinker::peripheralRequest("NFC_TARGET_ACTIVE",token_)!=0;}
    bool thinkcadReadEmulatedPage(uint8_t page,uint8_t* data){
        if(!ready_||!data||page>39)return false;
        int count=thinker::peripheralRequest("NFC_TARGET_PAGE",token_,std::to_string(page));
        for(int i=0;i<count;++i){auto byte=thinker::peripheralRead();if(i<4)data[i]=byte;}return count==4;
    }
    bool thinkcadWriteEmulatedPage(uint8_t page,const uint8_t* data){
        if(!ready_||!data||page<4||page>39)return false;
        return thinker::peripheralRequest("NFC_TARGET_WRITE",token_,std::to_string(page)+" "+thinker::peripheralHex(data,4))!=0;
    }
private:int token_=thinker::peripheralToken();uint8_t irq_,reset_;TwoWire* wire_;bool ready_=false;
};
