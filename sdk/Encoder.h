#pragma once
#include "Arduino.h"
// Polled quadrature adapter. Call read() often enough to observe each A/B edge.
class Encoder {
public:
    Encoder(uint8_t a,uint8_t b):a_(a),b_(b){pinMode(a_,INPUT_PULLUP);pinMode(b_,INPUT_PULLUP);}
    long read(){const int phase=(digitalRead(a_)<<1)|digitalRead(b_);if(previous_>=0){static const int delta[]={0,-1,1,0,1,0,0,-1,-1,0,0,1,0,1,-1,0};position_+=delta[(previous_<<2)|phase];}previous_=phase;return position_;}
    void write(long position){position_=position;previous_=-1;}
private:uint8_t a_,b_;int previous_=-1;long position_=0;
};
