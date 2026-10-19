#pragma once
#include "PeripheralProtocol.h"
#include "Stream.h"
constexpr int I2S_PHILIPS_MODE=0,I2S_RIGHT_JUSTIFIED_MODE=1,I2S_LEFT_JUSTIFIED_MODE=2;
class I2SClass:public Stream {
    int bclk_=SCK,ws_=SS,dout_=MOSI,din_=-1;unsigned rate_=48000,bits_=16,channels_=2;bool ready_=false;
public:
    using Print::write;
    void setPins(int bclk,int ws,int dout,int din=-1,int=-1){if(ready_)throw std::runtime_error("End I2S before changing pins");bclk_=bclk;ws_=ws;dout_=dout;din_=din;}
    void setSCK(int pin){bclk_=pin;}void setFS(int pin){ws_=pin;}void setDATA(int pin){dout_=pin;}
    int begin(int mode,long rate,int bits){if(mode!=I2S_PHILIPS_MODE)return 0;return start(rate,bits,2);}
    bool begin(int mode,unsigned rate,int bits,int slots,int=3){return mode==0&&start(rate,bits,slots);}
    bool start(unsigned rate,unsigned bits,unsigned channels){if(bits!=16&&bits!=24&&bits!=32)return false;ready_=thinker::request("I2S_BEGIN "+std::to_string(bclk_)+" "+std::to_string(ws_)+" "+std::to_string(dout_)+" "+std::to_string(din_)+" "+std::to_string(rate)+" "+std::to_string(bits)+" "+std::to_string(channels));rate_=rate;bits_=bits;channels_=channels;return ready_;}
    void end(){if(ready_)std::cout<<"I2S_END"<<std::endl;ready_=false;}
    int available()override{return ready_&&din_>=0?(bits_/8)*channels_:0;}
    int read()override{std::array<uint8_t,8> frame{};auto length=(bits_/8)*channels_;if(readBytes(frame.data(),length)!=length)return -1;std::uint32_t value=0;for(unsigned i=0;i<bits_/8;++i)value|=std::uint32_t(frame[i])<<(i*8);if(bits_<32&&(value&(1u<<(bits_-1))))value|=~((1u<<bits_)-1);return std::int32_t(value);}
    int peek()override{return -1;}void flush()override{}
    std::size_t readBytes(uint8_t* data,std::size_t length){if(!ready_||din_<0||(!data&&length))return 0;auto stride=(bits_/8)*channels_;if(length%stride)throw std::invalid_argument("I2S reads require whole PCM frames");std::size_t done=0;while(done<length){auto count=std::min<std::size_t>(length-done,8192/stride*stride);thinker::advance(std::uint64_t(count/stride)*1000000/rate_);int result=thinker::request("I2S_READ "+std::to_string(count));if(result<0||std::size_t(result)>count)throw std::runtime_error("Invalid I2S read response");for(int i=0;i<result;++i)data[done+i]=uint8_t(thinker::peripheralRead());done+=result;if(std::size_t(result)!=count)break;}return done;}
    std::size_t readBytes(char* data,std::size_t length){return readBytes(reinterpret_cast<uint8_t*>(data),length);}
    std::size_t write(uint8_t value)override{return write(&value,1);}
    std::size_t write(const uint8_t* data,std::size_t length)override{
        if(!ready_||(!data&&length))return 0;const auto stride=(bits_/8)*channels_;if(length%stride)throw std::invalid_argument("I2S writes require whole PCM frames");std::size_t done=0;while(done<length){auto count=std::min<std::size_t>(length-done,8192/stride*stride);int result=thinker::request("I2S_WRITE "+thinker::peripheralHex(data+done,count));if(result<0||std::size_t(result)>count)throw std::runtime_error("Invalid I2S write response");done+=result;thinker::advance(std::uint64_t(result/stride)*1000000/rate_);if(std::size_t(result)!=count)break;}return done;
    }
    std::size_t write(const void* data,std::size_t length){return write(static_cast<const uint8_t*>(data),length);}
    std::size_t write(int32_t sample){std::array<uint8_t,8> frame{};for(unsigned channel=0;channel<channels_;++channel)for(unsigned i=0;i<bits_/8;++i)frame[channel*(bits_/8)+i]=uint8_t(std::uint32_t(sample)>>(i*8));return write(frame.data(),(bits_/8)*channels_);}
};
inline I2SClass I2S;
