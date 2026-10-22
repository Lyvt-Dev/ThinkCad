#include "Arduino.h"
#include "IRremote.hpp"
#include <sstream>
#include <vector>
#include <iostream>
std::vector<uint64_t> ticks;
void tick(){ticks.push_back(thinker::clockUs);}
void check(bool v,const char* message){if(!v)throw std::runtime_error(message);}
int main(){thinker::standalone=true;std::ostringstream output;auto* saved=std::cout.rdbuf(output.rdbuf());
 check(B11111==31&&B00000101==5,"Arduino binary constants");pinMode(0,INPUT_PULLDOWN);
 check(ledcSetup(0,38000,8)==38000,"LEDC setup");ledcAttachPin(2,0);ledcWrite(0,85);
 check(output.str().find("PWM 2 85 255 38000")!=std::string::npos,"LEDC pin configuration");
 check(ledcAttach(4,56000,10),"LEDC pin API attach");check(ledcWrite(4,341),"LEDC pin API write");check(ledcRead(4)==341&&ledcReadFreq(4)==56000,"LEDC readback");
 auto* timer=timerBegin(1000000);timerAttachInterrupt(timer,tick);timerAlarm(timer,37,true,0);delayMicroseconds(200);timerEnd(timer);
 check(ticks==std::vector<uint64_t>({37,74,111,148,185}),"Exact timer virtual deadlines");check(micros()==200&&millis()==0,"Virtual microseconds");delay(1);check(micros()==1200&&millis()==1,"Virtual milliseconds");
 thinker::clockUs=(1ull<<32)-10;delayMicroseconds(20);check(micros()==10,"Arduino micros wraps at 32 bits");
 std::cout.rdbuf(saved);std::cout<<"ESP32 LEDC, timer deadlines, pull-down and Arduino constants passed\n";
}
