#pragma once
// Host callbacks at explicit Arduino API/time barriers, never CPU instructions.
constexpr int CHANGE=1,FALLING=2,RISING=3,NOT_AN_INTERRUPT=-1;
#ifndef IRAM_ATTR
#define IRAM_ATTR
#endif
#ifndef ICACHE_RAM_ATTR
#define ICACHE_RAM_ATTR
#endif
namespace thinker {
inline std::map<int,std::function<void()>> irqHandlers;
inline std::vector<int> irqPending;
inline bool irqEnabled=true,irqDispatching=false;
inline bool interruptsRegistered(){return !irqHandlers.empty();}
inline void dispatchInterrupts(){
    if(irqHandlers.empty()||!irqEnabled||irqDispatching||standalone)return;irqDispatching=true;
    try{
        const int count=request("IRQ_POLL");if(count<0||count>49)throw std::runtime_error("Invalid interrupt count");for(int i=0;i<count;++i){int pin=-1;if(!(std::cin>>pin))throw std::runtime_error("Invalid interrupt response");validatePin(pin);if(irqHandlers.contains(pin)&&std::find(irqPending.begin(),irqPending.end(),pin)==irqPending.end())irqPending.push_back(pin);}
        unsigned budget=49;while(irqEnabled&&!irqPending.empty()&&budget--){const int pin=irqPending.front();irqPending.erase(irqPending.begin());const auto handler=irqHandlers.find(pin);if(handler!=irqHandlers.end()){const auto callback=handler->second;callback();}}
    }catch(...){irqDispatching=false;throw;}irqDispatching=false;
}
inline int interruptPin(int number){
#if defined(THINKERCAD_ARDUINO_UNO)||defined(THINKERCAD_ARDUINO_NANO)
    if(number<0||number>1)throw std::invalid_argument("Uno/Nano only have interrupts 0 and 1 (D2/D3)");return number+2;
#else
    validatePin(number);
#if !defined(THINKERCAD_ESP32)&&!defined(THINKERCAD_ESP32_S3)&&!defined(THINKERCAD_GENERIC_BOARD)
    if(number==16)throw std::invalid_argument("ESP8266 GPIO16 has no external interrupt");
#endif
    return number;
#endif
}
inline void attachCallback(int number,std::function<void()> callback,int mode){const int pin=interruptPin(number);if(!callback||mode<0||mode>3)throw std::invalid_argument("Invalid interrupt callback or mode");if(!standalone&&request("IRQ_ATTACH "+std::to_string(pin)+" "+std::to_string(mode))!=1)throw std::runtime_error("GPIO interrupt unavailable");irqHandlers[pin]=std::move(callback);}
}
inline int digitalPinToInterrupt(int pin){
#if defined(THINKERCAD_ARDUINO_UNO)||defined(THINKERCAD_ARDUINO_NANO)
    return pin==2?0:pin==3?1:NOT_AN_INTERRUPT;
#else
    try{return thinker::interruptPin(pin);}catch(const std::invalid_argument&){return NOT_AN_INTERRUPT;}
#endif
}
inline void attachInterrupt(int number,void (*callback)(),int mode){thinker::attachCallback(number,callback,mode);}
inline void attachInterruptArg(int number,void (*callback)(void*),void* argument,int mode){if(!callback)throw std::invalid_argument("Null interrupt callback");thinker::attachCallback(number,[callback,argument]{callback(argument);},mode);}
inline void detachInterrupt(int number){const auto pin=thinker::interruptPin(number);thinker::irqHandlers.erase(pin);std::erase(thinker::irqPending,pin);if(!thinker::standalone)thinker::request("IRQ_DETACH "+std::to_string(pin));}
inline void noInterrupts(){thinker::irqEnabled=false;}
inline void interrupts(){thinker::irqEnabled=true;thinker::dispatchInterrupts();}
