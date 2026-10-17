#pragma once
#include "Arduino.h"
#include <vector>
#include <cmath>
using neoPixelType=uint16_t;
constexpr neoPixelType NEO_RGB=0x06,NEO_RBG=0x09,NEO_GRB=0x52,NEO_GBR=0xa1,NEO_BRG=0x58,NEO_BGR=0xa4,NEO_KHZ800=0,NEO_KHZ400=0x100;
class Adafruit_NeoPixel {
    std::vector<uint8_t> bytes_;
    int16_t pin_=-1;
    uint8_t red_=1,green_=0,blue_=2,brightness_=255;
    neoPixelType type_=NEO_GRB;
    std::uint64_t end_=0;
    bool begun_=false,shown_=false;
public:
    Adafruit_NeoPixel()=default;
    Adafruit_NeoPixel(uint16_t count,int16_t pin=6,neoPixelType type=NEO_GRB+NEO_KHZ800){setPin(pin);updateType(type);updateLength(count);}
    bool begin(){if(pin_<0)return false;pinMode(pin_,OUTPUT);digitalWrite(pin_,LOW);begun_=true;return true;}
    void setPin(int16_t pin){if(pin>=0)thinker::validatePin(pin);pin_=pin;if(begun_&&pin_>=0){pinMode(pin_,OUTPUT);digitalWrite(pin_,LOW);}}
    int16_t getPin()const{return pin_;}
    void updateLength(uint16_t count){if(count>1024)throw std::invalid_argument("Maximum 1024 native NeoPixels");bytes_.assign(std::size_t(count)*3,0);}
    void updateType(neoPixelType type){const auto w=(type>>6)&3,r=(type>>4)&3,g=(type>>2)&3,b=type&3;if(w!=r||r>2||g>2||b>2||r==g||r==b||g==b||(type&~0x1ff))throw std::invalid_argument("Native WS2812B supports RGB pixels only");type_=type;red_=r;green_=g;blue_=b;}
    uint16_t numPixels()const{return uint16_t(bytes_.size()/3);}
    uint8_t* getPixels()const{return const_cast<uint8_t*>(bytes_.data());}
    uint16_t numBytes()const{return uint16_t(bytes_.size());}
    uint8_t getBrightness()const{return brightness_;}
    void setBrightness(uint8_t value){if(value==brightness_)return;for(auto& byte:bytes_)byte=brightness_?uint8_t(std::min(255u,unsigned(byte)*(unsigned(value)+1)/(unsigned(brightness_)+1))):0;brightness_=value;}
    void clear(){std::fill(bytes_.begin(),bytes_.end(),0);}
    static uint32_t Color(uint8_t red,uint8_t green,uint8_t blue){return (uint32_t(red)<<16)|(uint32_t(green)<<8)|blue;}
    static uint32_t Color(uint8_t red,uint8_t green,uint8_t blue,uint8_t white){if(white)throw std::invalid_argument("WS2812B has no white channel");return Color(red,green,blue);}
    void setPixelColor(uint16_t index,uint8_t red,uint8_t green,uint8_t blue){if(index>=numPixels())return;auto* pixel=bytes_.data()+index*3;auto scale=[&](uint8_t value){return uint8_t((unsigned(value)*(unsigned(brightness_)+1))>>8);};pixel[red_]=scale(red);pixel[green_]=scale(green);pixel[blue_]=scale(blue);}
    void setPixelColor(uint16_t index,uint8_t red,uint8_t green,uint8_t blue,uint8_t white){if(white)throw std::invalid_argument("WS2812B has no white channel");setPixelColor(index,red,green,blue);}
    void setPixelColor(uint16_t index,uint32_t color){setPixelColor(index,uint8_t(color>>16),uint8_t(color>>8),uint8_t(color));}
    uint32_t getPixelColor(uint16_t index)const{if(index>=numPixels())return 0;const auto* p=bytes_.data()+index*3;auto unscale=[&](uint8_t value){return brightness_==255?value:uint8_t((unsigned(value)<<8)/(unsigned(brightness_)+1));};return Color(unscale(p[red_]),unscale(p[green_]),unscale(p[blue_]));}
    void fill(uint32_t color=0,uint16_t first=0,uint16_t count=0){const auto end=count?std::min<unsigned>(numPixels(),unsigned(first)+count):numPixels();for(unsigned i=first;i<end;++i)setPixelColor(i,color);}
    bool canShow()const{return !shown_||thinker::clockUs-end_>=300;}
    void show(){if(!begun_||pin_<0||bytes_.empty())return;if(!canShow())delayMicroseconds(unsigned(300-(thinker::clockUs-end_)));
#ifdef THINKERCAD_EMULATION
        delayMicroseconds(unsigned(bytes_.size()*(type_&NEO_KHZ400?20:10)));
#endif
        static constexpr char hex[]="0123456789abcdef";std::string frame;frame.reserve(bytes_.size()*2);for(auto byte:bytes_){frame+=hex[byte>>4];frame+=hex[byte&15];}thinker::request("NEOPIXEL "+std::to_string(pin_)+" "+std::to_string(type_&NEO_KHZ400?400000:800000)+" "+frame);end_=thinker::clockUs;shown_=true;
    }
    static uint8_t gamma8(uint8_t value){return uint8_t(std::lround(std::pow(value/255.,2.6)*255));}
    static uint32_t gamma32(uint32_t color){return (uint32_t(gamma8(color>>24))<<24)|Color(gamma8(color>>16),gamma8(color>>8),gamma8(color));}
    static uint8_t sine8(uint8_t value){return uint8_t(std::lround((std::sin(value*6.283185307179586/256)+1)*127.5));}
    static uint32_t ColorHSV(uint16_t hue,uint8_t saturation=255,uint8_t value=255){const double h=hue*6./65536;const int sector=int(h);const double f=h-sector,v=value/255.,s=saturation/255.,p=v*(1-s),q=v*(1-s*f),t=v*(1-s*(1-f));const double channels[][3]={{v,t,p},{q,v,p},{p,v,t},{p,q,v},{t,p,v},{v,p,q}};return Color(uint8_t(std::lround(channels[sector][0]*255)),uint8_t(std::lround(channels[sector][1]*255)),uint8_t(std::lround(channels[sector][2]*255)));}
    void rainbow(uint16_t firstHue=0,int8_t repetitions=1,uint8_t saturation=255,uint8_t brightness=255,bool gammify=true){for(unsigned i=0;i<numPixels();++i){const auto color=ColorHSV(uint16_t(firstHue+(std::int64_t(i)*65536*repetitions)/numPixels()),saturation,brightness);setPixelColor(i,gammify?gamma32(color):color);}}
};
