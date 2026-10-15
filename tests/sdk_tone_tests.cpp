#include "Arduino.h"
#include <sstream>
#include <iostream>
int main(){
    std::ostringstream output;auto* previous=std::cout.rdbuf(output.rdbuf());thinker::standalone=true;
    tone(D5,1000,7);delay(10);
    const auto finite=output.str();
    bool ok=finite.find("TONE "+std::to_string(D5)+" 1000")!=std::string::npos&&finite.find("TIME 7000\nNOTONE "+std::to_string(D5))!=std::string::npos&&thinker::clockUs==10000&&thinker::toneDeadlines.empty();
    tone(D5,440,1);tone(D5,880);delay(2);ok=ok&&thinker::toneDeadlines.empty();
    noTone(D5);tone(D5,1000,5);digitalWrite(D5,LOW);ok=ok&&thinker::toneDeadlines.empty();
    tone(D5,0);ok=ok&&thinker::toneDeadlines.empty();
    bool rejected=false;try{tone(D5,22000);}catch(const std::invalid_argument&){rejected=true;}ok=ok&&rejected;
    std::cout.rdbuf(previous);std::cout<<(ok?"Tone duration, cancellation, and protocol passed\n":"Tone SDK failed\n");return ok?0:1;
}
