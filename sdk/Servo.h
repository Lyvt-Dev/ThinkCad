#pragma once
#include "Arduino.h"
class Servo {
public:
    std::uint8_t attach(int pin,int minimum=544,int maximum=2400){if(minimum<500||maximum>2500||minimum>=maximum)throw std::invalid_argument("Servo pulse limits outside 500..2500 us");if(attached())detach();pin_=pin;min_=minimum;max_=maximum;pinMode(pin_,OUTPUT);write(90);return 0;}
    void detach(){if(attached()){std::cout<<"SERVO_DETACH "<<pin_<<std::endl;pin_=-1;}}
    bool attached()const{return pin_>=0;}
    void write(int value){if(value>=500)writeMicroseconds(value);else writeMicroseconds(min_+std::clamp(value,0,180)*(max_-min_)/180);}
    void writeMicroseconds(int value){if(!attached())return;pulse_=std::clamp(value,min_,max_);std::cout<<"SERVO "<<pin_<<' '<<pulse_<<std::endl;}
    int read()const{return std::clamp((pulse_-min_)*180/(max_-min_),0,180);}int readMicroseconds()const{return pulse_;}
private:int pin_=-1,min_=544,max_=2400,pulse_=1472;
};
