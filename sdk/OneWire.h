#pragma once
#include "Arduino.h"
// Native protocol bridge. No target-specific GPIO registers or CPU assembly.
class OneWire {
    int pin_=-1;
    unsigned searchIndex_=0;
    int request(int operation,int value=0){if(pin_<0)return operation==2?255:0;return thinker::request("ONEWIRE "+std::to_string(pin_)+" "+std::to_string(operation)+" "+std::to_string(value));}
    void time(unsigned us){
#ifdef THINKERCAD_EMULATION
        delayMicroseconds(us);
#else
        (void)us;
#endif
    }
public:
    OneWire()=default;
    explicit OneWire(uint8_t pin){begin(pin);}
    void begin(uint8_t pin){thinker::validatePin(pin);pin_=pin;pinMode(pin_,INPUT);reset_search();}
    uint8_t reset(){time(960);return request(0);}
    void select(const uint8_t address[8]){write(0x55);write_bytes(address,8);}
    void skip(){write(0xcc);}
    void write(uint8_t value,uint8_t power=0){(void)power;request(1,value);time(480);}
    void write_bytes(const uint8_t* bytes,uint16_t count,bool power=false){for(unsigned i=0;i<count;++i)write(bytes[i],power&&i+1==count);}
    uint8_t read(){const auto result=request(2);time(480);return result;}
    void read_bytes(uint8_t* bytes,uint16_t count){for(unsigned i=0;i<count;++i)bytes[i]=read();}
    void write_bit(uint8_t bit){request(7,bit!=0);time(60);}
    uint8_t read_bit(){const auto result=request(3);time(60);return result;}
    void depower(){request(4);}
    void reset_search(){searchIndex_=0;}
    void target_search(uint8_t family){searchIndex_=family==0x28?0:64;}
    bool search(uint8_t* address,bool normal_search=true){if(!address||searchIndex_>=64)return false;for(unsigned i=0;i<8;++i){const int result=request(5,int(searchIndex_*8+i)+(normal_search?0:4096));if(result<0)return false;address[i]=uint8_t(result);}++searchIndex_;time(64*60);return true;}
    static uint8_t crc8(const uint8_t* bytes,uint8_t count){uint8_t crc=0;for(unsigned i=0;i<count;++i){crc^=bytes[i];for(int bit=0;bit<8;++bit)crc=(crc>>1)^((crc&1)?0x8c:0);}return crc;}
    static uint16_t crc16(const uint8_t* bytes,uint16_t count,uint16_t crc=0){for(unsigned i=0;i<count;++i){crc^=bytes[i];for(int bit=0;bit<8;++bit)crc=(crc>>1)^((crc&1)?0xa001:0);}return crc;}
    static bool check_crc16(const uint8_t* bytes,uint16_t count,const uint8_t* inverted,uint16_t initial=0){const auto crc=uint16_t(~crc16(bytes,count,initial));return inverted&&inverted[0]==uint8_t(crc)&&inverted[1]==uint8_t(crc>>8);}
};
