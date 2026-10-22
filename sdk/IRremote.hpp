#pragma once
#include "PeripheralProtocol.h"
#include "irwaveform.hpp"
constexpr uint8_t IRDATA_FLAGS_IS_REPEAT=1,IRDATA_FLAGS_TOGGLE_BIT=8,IRDATA_FLAGS_EXTRA_INFO=0x10,IRDATA_FLAGS_WAS_OVERFLOW=0x40,IRDATA_FLAGS_IS_MSB_FIRST=0x80,IRDATA_FLAGS_IS_LSB_FIRST=0;
constexpr int DISABLE_LED_FEEDBACK=0,ENABLE_LED_FEEDBACK=1;
constexpr unsigned MICROS_PER_TICK=50,RAW_BUFFER_LENGTH=258;
enum decode_type_t {UNKNOWN,NEC,SAMSUNG,SONY,RC5,RC6,KASEIKYO,PANASONIC};
struct irparams_struct {uint16_t rawlen=0;uint16_t rawbuf[RAW_BUFFER_LENGTH]{};};
struct IRData {decode_type_t protocol=UNKNOWN;uint16_t address=0,command=0,extra=0;uint64_t decodedRawData=0;uint16_t numberOfBits=0;uint8_t flags=0;irparams_struct* rawDataPtr=nullptr;uint32_t initialGapTicks=0;};
class IRsend {
public:
 void begin(int pin,bool=false,int=-1){pin_=pin;pinMode(pin,OUTPUT);digitalWrite(pin,LOW);if(!thinker::peripheralRequest("IR_TX_BEGIN",token_,std::to_string(pin)))throw std::runtime_error("IR LED not reachable: use a series resistor, correct polarity and GND");}
 void begin(){
#ifdef IR_SEND_PIN
 begin(IR_SEND_PIN);
#else
 throw std::invalid_argument("Use IrSender.begin(pin), or define IR_SEND_PIN before including IRremote.hpp");
#endif
 }
 void enableIROut(uint_fast8_t khz){if(khz<20||khz>60)throw std::invalid_argument("IR carrier must be 20..60 kHz");hz_=unsigned(khz)*1000;}
 void setDutyCycle(uint8_t percent){if(percent<10||percent>60)throw std::invalid_argument("IR carrier duty must be 10..60 percent");duty_=percent/100.;}
 void sendNEC(uint16_t a,uint16_t c,int_fast8_t r=0){send(thinkir::Nec,a,c,r);}
 void sendNECRaw(uint32_t raw,int_fast8_t repeats=0){if(repeats<0||repeats>8)throw std::invalid_argument("NEC raw repeats must be 0..8");for(int i=0;i<=repeats;++i){thinkir::Wave w;if(i)w=thinkir::encode(thinkir::Nec,0,0,true);else{thinkir::pulse(w,true,9000);thinkir::pulse(w,false,4500);thinkir::distance(w,raw,32,560,560,1690);}if(hz_)w.hz=hz_;auto duration=w.duration();transmit(w);if(i<repeats&&duration<110000)thinker::advance(110000-duration);}}
 void sendSamsung(uint16_t a,uint16_t c,int_fast8_t r=0){send(thinkir::Samsung,a,c,r);}
 void sendSony(uint16_t a,uint8_t c,int_fast8_t r=0,uint8_t bits=12){send(thinkir::Sony,a,c,r,bits);}
 void sendRC5(uint8_t a,uint8_t c,int_fast8_t r=0,bool autoToggle=true){if(autoToggle)toggle_=!toggle_;send(thinkir::Rc5,a,c,r);}
 void sendRC6(uint8_t a,uint8_t c,int_fast8_t r=0,bool autoToggle=true){if(autoToggle)toggle_=!toggle_;send(thinkir::Rc6,a,c,r);}
 void sendKaseikyo(uint16_t a,uint8_t c,int_fast8_t r,uint16_t vendor){send(thinkir::Kaseikyo,a,c,r,0,vendor);}
 void sendPanasonic(uint16_t a,uint8_t c,int_fast8_t r=0){sendKaseikyo(a,c,r,0x2002);}
 void sendRaw(const uint16_t* buffer,uint_fast16_t length,uint_fast8_t khz){if(!buffer||!length||length>256)throw std::invalid_argument("IR raw frame requires 1..256 durations");if(khz<20||khz>60)throw std::invalid_argument("IR carrier must be 20..60 kHz");thinkir::Wave w;w.hz=unsigned(khz)*1000;w.duty=duty_;for(unsigned i=0;i<length;++i){if(!buffer[i]||buffer[i]>20000)throw std::invalid_argument("IR raw durations must be 1..20000 microseconds");thinkir::pulse(w,i%2==0,buffer[i]);}transmit(w);}
private:
 void transmit(thinkir::Wave w){if(pin_<0)throw std::runtime_error("Call IrSender.begin(pin) first");if(w.pulses.empty()||w.duration()>250000||w.pulses.size()>256)throw std::invalid_argument("IR frame exceeds bounded capture");w.duty=duty_;std::string args=std::to_string(w.hz)+" "+std::to_string(unsigned(w.duty*1000))+" "+std::to_string(w.pulses.size())+" "+std::to_string(w.pulses.front().mark?1:0);for(auto p:w.pulses)args+=" "+std::to_string(p.us);if(!thinker::peripheralRequest("IR_WAVE",token_,args))throw std::runtime_error("IR transmitter disconnected or not forward biased");thinker::waveformStepLimit=std::min<uint64_t>(thinker::waveformStepLimit,std::max<uint64_t>(1,500000000ull/w.hz));thinker::advance(w.duration());}
 void send(thinkir::Protocol p,uint16_t a,uint16_t c,int repeats,unsigned bits=0,uint16_t vendor=0x2002){if(repeats==-1&&p==thinkir::Nec){auto w=thinkir::encode(p,a,c,true);if(hz_)w.hz=hz_;transmit(w);return;}if(repeats<0||repeats>8)throw std::invalid_argument("IR repeat count must be 0..8");for(int i=0;i<=repeats;++i){auto w=thinkir::encode(p,a,c,i&&p==thinkir::Nec,bits,toggle_,vendor);if(hz_)w.hz=hz_;auto duration=w.duration();transmit(w);if(i<repeats){const unsigned period=p==thinkir::Sony?45000:p==thinkir::Rc5?113792:p==thinkir::Rc6?unsigned(duration+107000):p==thinkir::Kaseikyo?130000:110000;if(duration<period)thinker::advance(period-duration);}}}
 int token_=thinker::peripheralToken(),pin_=-1;unsigned hz_=0;double duty_=1./3;bool toggle_=false;
};
class IRrecv {
public:
 IRData decodedIRData;
 void begin(int pin,bool=false,int=-1){pinMode(pin,INPUT);if(!thinker::peripheralRequest("IR_RX_BEGIN",token_,std::to_string(pin)))throw std::runtime_error("IR receiver not connected: check VCC/GND/OUT");ready_=true;}
 void begin(){
#ifdef IR_RECEIVE_PIN
 begin(IR_RECEIVE_PIN);
#else
 throw std::invalid_argument("Use IrReceiver.begin(pin), or define IR_RECEIVE_PIN");
#endif
 }
 bool decode(){if(!ready_||!thinker::peripheralRequest("IR_CAPTURE",token_))return false;decodedIRData.address=thinker::peripheralRead();decodedIRData.command=thinker::peripheralRead();decodedIRData.flags=thinker::peripheralRead();decodedIRData.protocol=static_cast<decode_type_t>(thinker::peripheralRead());decodedIRData.numberOfBits=thinker::peripheralRead();decodedIRData.decodedRawData=0;for(unsigned i=0;i<4;++i)decodedIRData.decodedRawData|=uint64_t(uint16_t(thinker::peripheralRead()))<<(i*16);decodedIRData.extra=0;if(decodedIRData.protocol==KASEIKYO){decodedIRData.extra=uint16_t(decodedIRData.decodedRawData);if(decodedIRData.extra==0x2002)decodedIRData.protocol=PANASONIC;else decodedIRData.flags|=IRDATA_FLAGS_EXTRA_INFO;}if(decodedIRData.protocol==RC5||decodedIRData.protocol==RC6)decodedIRData.flags|=IRDATA_FLAGS_IS_MSB_FIRST;raw_.rawlen=thinker::peripheralRead();if(raw_.rawlen>RAW_BUFFER_LENGTH)throw std::runtime_error("IR capture overflow");for(unsigned i=0;i<raw_.rawlen;++i)raw_.rawbuf[i]=thinker::peripheralRead();decodedIRData.rawDataPtr=&raw_;return true;}
 void resume(){if(ready_)thinker::peripheralRequest("IR_RESUME",token_);}void stop(){if(ready_)thinker::peripheralRequest("IR_CONTROL",token_,"0");ready_=false;}void start(){thinker::peripheralRequest("IR_CONTROL",token_,"1");ready_=true;}bool isIdle()const{return !ready_||thinker::peripheralRequest("IR_IDLE",token_)!=0;}
private:int token_=thinker::peripheralToken();bool ready_=false;irparams_struct raw_;
};
inline IRsend IrSender;inline IRrecv IrReceiver;
