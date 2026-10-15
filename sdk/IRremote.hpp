#pragma once
#include "PeripheralProtocol.h"
constexpr uint8_t IRDATA_FLAGS_IS_REPEAT=1;
constexpr int DISABLE_LED_FEEDBACK=0,ENABLE_LED_FEEDBACK=1;enum decode_type_t {UNKNOWN,NEC};
struct IRData {decode_type_t protocol=UNKNOWN;uint16_t address=0,command=0;uint32_t decodedRawData=0;uint16_t numberOfBits=32;uint8_t flags=0;};
class IRsend {
public:
    void begin(int pin,bool=false,int=-1){pin_=pin;pinMode(pin,OUTPUT);digitalWrite(pin,LOW);if(!thinker::peripheralRequest("IR_TX_BEGIN",token_,std::to_string(pin)))throw std::runtime_error("IR LED not reachable: use a series resistor, correct polarity and GND");}
    void sendNEC(uint16_t address,uint8_t command,int repeats=0){if(pin_<0||repeats<0||repeats>8)throw std::invalid_argument("Initialize IR sender; repeats must be 0..8");for(int i=0;i<=repeats;++i){digitalWrite(pin_,HIGH);thinker::peripheralRequest("IR_SEND",token_,std::to_string(address)+" "+std::to_string(command)+" "+std::to_string(i?IRDATA_FLAGS_IS_REPEAT:0));delay(68);digitalWrite(pin_,LOW);if(i<repeats)delay(40);}}
private:int token_=thinker::peripheralToken(),pin_=-1;
};
class IRrecv {
public:
    IRData decodedIRData;
    void begin(int pin,bool=false,int=-1){pinMode(pin,INPUT);if(!thinker::peripheralRequest("IR_RX_BEGIN",token_,std::to_string(pin)))throw std::runtime_error("IR receiver not connected: check VCC/GND/OUT");ready_=true;}
    bool decode(){if(!ready_)return false;if(!thinker::peripheralRequest("IR_DECODE",token_))return false;decodedIRData.address=thinker::peripheralRead();decodedIRData.command=thinker::peripheralRead();decodedIRData.flags=thinker::peripheralRead();decodedIRData.protocol=NEC;uint32_t a=decodedIRData.address;uint32_t upper=a>255?a>>8:(~a)&255;decodedIRData.decodedRawData=(a&255)|(upper<<8)|(uint32_t(decodedIRData.command)<<16)|((~uint32_t(decodedIRData.command)&255)<<24);return true;}
    void resume(){if(ready_)thinker::peripheralRequest("IR_RESUME",token_);}void stop(){ready_=false;}void start(){ready_=true;}
private:int token_=thinker::peripheralToken();bool ready_=false;
};
inline IRsend IrSender;inline IRrecv IrReceiver;
