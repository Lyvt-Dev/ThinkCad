#pragma once
#include "Arduino.h"
#include <cmath>
constexpr uint8_t DHT11=11,DHT12=12,DHT21=21,DHT22=22,AM2301=21;
class DHT {
    uint8_t pin_,type_;
    std::array<uint8_t,5> data_{};
    bool valid_=false;
    std::uint64_t last_=0;
public:
    DHT(uint8_t pin,uint8_t type,uint8_t count=6):pin_(pin),type_(type){(void)count;thinker::validatePin(pin);}
    void begin(uint8_t pullTime=55){(void)pullTime;pinMode(pin_,INPUT);valid_=false;}
    bool read(bool force=false){
        if(type_!=DHT22&&type_!=DHT21)return false;
        if(!force&&valid_&&thinker::clockUs-last_<2000000)return true;
        valid_=false;for(unsigned i=0;i<data_.size();++i){const int value=thinker::request("DHT_READ "+std::to_string(pin_)+" "+std::to_string(i));if(value<0||value>255)return false;data_[i]=uint8_t(value);}
        valid_=uint8_t(data_[0]+data_[1]+data_[2]+data_[3])==data_[4];last_=thinker::clockUs;
#ifdef THINKERCAD_EMULATION
        delayMicroseconds(5200);
#endif
        return valid_;
    }
    float readTemperature(bool fahrenheit=false,bool force=false){if(!read(force))return NAN;float value=((data_[2]&0x7f)*256+data_[3])*.1f;if(data_[2]&0x80)value=-value;return fahrenheit?convertCtoF(value):value;}
    float readHumidity(bool force=false){return read(force)?(data_[0]*256+data_[1])*.1f:NAN;}
    float convertCtoF(float value){return value*1.8f+32;}
    float convertFtoC(float value){return (value-32)*.55555556f;}
    float computeHeatIndex(bool fahrenheit=true){return computeHeatIndex(readTemperature(fahrenheit),readHumidity(),fahrenheit);}
    float computeHeatIndex(float temperature,float humidity,bool fahrenheit=true){
        const float t=fahrenheit?temperature:convertCtoF(temperature),h=humidity;
        float result=.5f*(t+61+(t-68)*1.2f+h*.094f);
        if(result>79){result=-42.379f+2.04901523f*t+10.14333127f*h-.22475541f*t*h-.00683783f*t*t-.05481717f*h*h+.00122874f*t*t*h+.00085282f*t*h*h-.00000199f*t*t*h*h;if(h<13&&t>=80&&t<=112)result-=(13-h)*.25f*std::sqrt((17-std::abs(t-95))*.05882353f);else if(h>85&&t>=80&&t<=87)result+=(h-85)*.1f*(87-t)*.2f;}
        return fahrenheit?result:convertFtoC(result);
    }
};
