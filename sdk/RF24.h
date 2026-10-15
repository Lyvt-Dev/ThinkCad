#pragma once
#include "PeripheralProtocol.h"
#include "SPI.h"
#include <cstring>
enum rf24_datarate_e {RF24_1MBPS,RF24_2MBPS,RF24_250KBPS};enum rf24_pa_dbm_e {RF24_PA_MIN,RF24_PA_LOW,RF24_PA_HIGH,RF24_PA_MAX};enum rf24_crclength_e {RF24_CRC_DISABLED,RF24_CRC_8,RF24_CRC_16};
class RF24 {
public:
    RF24(uint16_t ce,uint16_t csn,uint32_t=10000000):ce_(ce),csn_(csn){}
    bool begin(SPIClass* bus=&SPI){bus_=bus;if(!bus_->ready())bus_->begin();pinMode(ce_,OUTPUT);pinMode(csn_,OUTPUT);digitalWrite(ce_,LOW);digitalWrite(csn_,HIGH);ready_=thinker::peripheralRequest("RF_BEGIN",token_,std::to_string(bus_->sck())+" "+std::to_string(bus_->miso())+" "+std::to_string(bus_->mosi())+" "+std::to_string(csn_)+" "+std::to_string(ce_));if(ready_)configure();return ready_;}
    bool isChipConnected()const{return ready_;}
    void setChannel(uint8_t channel){if(channel>125)throw std::invalid_argument("nRF24 channel must be 0..125");channel_=channel;if(ready_)configure();}uint8_t getChannel()const{return channel_;}
    bool setDataRate(rf24_datarate_e rate){if(rate!=RF24_250KBPS&&rate!=RF24_1MBPS&&rate!=RF24_2MBPS)return false;rate_=rate==RF24_250KBPS?250:rate==RF24_2MBPS?2000:1000;if(ready_)configure();return true;}
    void setCRCLength(rf24_crclength_e crc){crc_=crc;if(ready_)configure();}void disableCRC(){setCRCLength(RF24_CRC_DISABLED);}
    void setAutoAck(bool enabled){ack_=enabled;if(ready_)configure();}
    void setPALevel(uint8_t level,bool=true){if(level>RF24_PA_MAX)throw std::invalid_argument("Invalid RF24 PA level");} // No RF attenuation/noise model.
    void setRetries(uint8_t delay,uint8_t count){if(delay>15||count>15)throw std::invalid_argument("Invalid RF24 retry settings");retryDelay_=delay;retries_=count;}
    void setAddressWidth(uint8_t width){if(width!=5)throw std::invalid_argument("RF24 adapter supports five-byte addresses");}
    void setPayloadSize(uint8_t size){if(!size||size>32)throw std::invalid_argument("Payload must be 1..32 bytes");payload_=size;dynamic_=false;}uint8_t getPayloadSize()const{return payload_;}
    void enableDynamicPayloads(){dynamic_=true;}void disableDynamicPayloads(){dynamic_=false;}
    void openWritingPipe(const uint8_t* address){require();thinker::peripheralRequest("RF_DEST",token_,thinker::peripheralHex(address,5));}void openWritingPipe(uint64_t address){uint8_t a[5];for(int i=0;i<5;++i)a[i]=(address>>(8*i))&255;openWritingPipe(a);}
    void openReadingPipe(uint8_t pipe,const uint8_t* address){require();if(pipe>5)throw std::invalid_argument("Pipe must be 0..5");thinker::peripheralRequest("RF_PIPE",token_,std::to_string(pipe)+" "+thinker::peripheralHex(address,5));}void openReadingPipe(uint8_t pipe,uint64_t address){uint8_t a[5];for(int i=0;i<5;++i)a[i]=(address>>(8*i))&255;openReadingPipe(pipe,a);}
    void startListening(){listen(true);}void stopListening(){listen(false);}
    bool write(const void* data,uint8_t size){require();if(listening_||!data||!size||size>32)throw std::invalid_argument("RF24 write needs 1..32 bytes and TX mode");std::vector<uint8_t> bytes(static_cast<const uint8_t*>(data),static_cast<const uint8_t*>(data)+std::min<int>(size,dynamic_?32:payload_));if(!dynamic_)bytes.resize(payload_,0);bool ack=thinker::peripheralRequest("RF_SEND",token_,thinker::peripheralHex(bytes.data(),bytes.size()));delayMicroseconds((bytes.size()+9)*8000/rate_+130);if(!ack)delayMicroseconds((retryDelay_+1)*250*(retries_+1));return ack;}
    bool available(uint8_t* pipe=nullptr){require();int size=thinker::peripheralRequest("RF_AVAILABLE",token_);if(size){int p=thinker::peripheralRead();if(pipe)*pipe=p;lastSize_=size;}else lastSize_=0;return size!=0;}
    uint8_t getDynamicPayloadSize(){available();return lastSize_;}
    void read(void* buffer,uint8_t length){require();if(!buffer||!length||length>32)throw std::invalid_argument("Invalid RF24 receive buffer");int size=thinker::peripheralRequest("RF_READ",token_);auto* bytes=static_cast<uint8_t*>(buffer);std::memset(bytes,0,length);for(int i=0;i<size;++i){int b=thinker::peripheralRead();if(i<length)bytes[i]=b;}}
private:
    void require()const{if(!ready_)throw std::runtime_error("RF24 not initialized; check 3.3V/GND and SPI/CE/CSN wiring");}
    void configure(){thinker::peripheralRequest("RF_CONFIG",token_,std::to_string(channel_)+" "+std::to_string(rate_)+" "+std::to_string(crc_)+" "+std::to_string(ack_));}
    void listen(bool on){require();digitalWrite(ce_,on?HIGH:LOW);thinker::peripheralRequest("RF_LISTEN",token_,on?"1":"0");listening_=on;}
    int token_=thinker::peripheralToken(),ce_,csn_,channel_=76,rate_=1000,crc_=2,payload_=32,lastSize_=0,retryDelay_=5,retries_=15;SPIClass* bus_=nullptr;bool ready_=false,listening_=false,ack_=true,dynamic_=false;
};
