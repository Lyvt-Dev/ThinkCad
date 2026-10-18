#include "Arduino.h"
#include <sstream>
#include <iostream>
#include <stdexcept>
int calls=0;void handler(){++calls;}
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
int main(){std::ostringstream output;std::istringstream replies("1 1 4 0 0 1 0\n");auto* oldOut=std::cout.rdbuf(output.rdbuf());auto* oldIn=std::cin.rdbuf(replies.rdbuf());try{
    attachInterrupt(digitalPinToInterrupt(4),handler,CHANGE);noInterrupts();thinker::dispatchInterrupts();require(calls==0,"disabled interrupts do not poll or dispatch");interrupts();require(calls==1,"enabling interrupts dispatches pending host callback");thinker::dispatchInterrupts();require(calls==1,"empty queue does not repeat callback");detachInterrupt(4);require(thinker::irqHandlers.empty(),"detaching removes native callback");int argument=0;attachInterruptArg(4,[](void* p){++*static_cast<int*>(p);},&argument,RISING);detachInterrupt(4);require(argument==0,"argument callback is not invoked without an edge");
    std::cout.rdbuf(oldOut);std::cin.rdbuf(oldIn);std::cout<<"Native interrupt masking, pending callback and detach API checks passed\n";return 0;
}catch(const std::exception& e){std::cout.rdbuf(oldOut);std::cin.rdbuf(oldIn);std::cerr<<e.what()<<'\n';return 1;}}
