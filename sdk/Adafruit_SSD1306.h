#pragma once
#include "Wire.h"
#include "Adafruit_GFX.h"
#include <vector>
constexpr int SSD1306_SWITCHCAPVCC=2,SSD1306_EXTERNALVCC=1;
constexpr int SSD1306_BLACK=0,SSD1306_WHITE=1,SSD1306_INVERSE=2;
constexpr int SSD1306_DISPLAYOFF=0xae,SSD1306_DISPLAYON=0xaf,SSD1306_INVERTDISPLAY=0xa7,SSD1306_NORMALDISPLAY=0xa6;
class Adafruit_SSD1306:public Adafruit_GFX {
public:
    Adafruit_SSD1306(uint8_t w,uint8_t h,TwoWire* wire=&Wire,int8_t=-1,uint32_t=400000,uint32_t=100000):Adafruit_GFX(w,h),wire_(wire),pixels_(static_cast<std::size_t>(w)*h/8),presented_(pixels_){if(w!=128||(h!=32&&h!=64)||!wire)throw std::invalid_argument("SSD1306 supports 128x32 or 128x64 I2C in ThinkerCad");}
    bool begin(uint8_t,uint8_t address=0x3c,bool=true,bool periphBegin=true){if(address!=0x3c&&address!=0x3d)return false;address_=address;if(periphBegin&&!wire_->ready())wire_->begin();begun_=wire_->ready()&&thinker::request("OLED_BEGIN "+std::to_string(address_)+" "+std::to_string(HEIGHT))==1;return begun_;}
    void clearDisplay(){std::fill(pixels_.begin(),pixels_.end(),0);}
    void drawPixel(int16_t x,int16_t y,uint16_t color)override{
        if(x<0||x>=width()||y<0||y>=height())return;
        int16_t t;switch(getRotation()){case 1:t=x;x=WIDTH-y-1;y=t;break;case 2:x=WIDTH-x-1;y=HEIGHT-y-1;break;case 3:t=x;x=y;y=HEIGHT-t-1;break;}
        auto& b=pixels_.at(x+(y/8)*WIDTH);const auto mask=static_cast<uint8_t>(1u<<(y%8));if(color==SSD1306_INVERSE)b^=mask;else if(color)b|=mask;else b&=static_cast<uint8_t>(~mask);
    }
    bool getPixel(int16_t x,int16_t y)const{if(x<0||y<0||x>=width()||y>=height())return false;int16_t t;switch(getRotation()){case 1:t=x;x=WIDTH-y-1;y=t;break;case 2:x=WIDTH-x-1;y=HEIGHT-y-1;break;case 3:t=x;x=y;y=HEIGHT-t-1;break;}return pixels_[x+(y/8)*WIDTH]&(1u<<(y%8));}
    uint8_t* getBuffer(){return pixels_.data();}
    void invertDisplay(bool inverted)override{inverted_=inverted;if(begun_)publish();}
    void dim(bool dimmed){dimmed_=dimmed;if(begun_)publish();}
    void ssd1306_command(uint8_t command){if(command==SSD1306_DISPLAYOFF)enabled_=false;else if(command==SSD1306_DISPLAYON)enabled_=true;else if(command==SSD1306_INVERTDISPLAY)inverted_=true;else if(command==SSD1306_NORMALDISPLAY)inverted_=false;else throw std::invalid_argument("Raw SSD1306 command not modelled; use the graphics API");if(begun_)publish();}
    void startscrollright(uint8_t start,uint8_t stop){scroll(1,0,start,stop);}
    void startscrollleft(uint8_t start,uint8_t stop){scroll(-1,0,start,stop);}
    void startscrolldiagright(uint8_t start,uint8_t stop){scroll(1,1,start,stop);}
    void startscrolldiagleft(uint8_t start,uint8_t stop){scroll(-1,1,start,stop);}
    void stopscroll(){scrolling_=false;if(begun_)publish();}
    void display(){
        if(!begun_)throw std::logic_error("Call display.begin first");
        thinker::advance(((pixels_.size()+2)*9ull*1000000+wire_->clock()-1)/wire_->clock());
        presented_=pixels_;publish();
    }
private:
    TwoWire* wire_;std::vector<uint8_t> pixels_,presented_;uint8_t address_=0x3c;bool begun_=false,inverted_=false,enabled_=true,dimmed_=false,scrolling_=false;int scrollX_=0,scrollY_=0,firstPage_=0,lastPage_=7;std::uint64_t scrollStarted_=0;int lastScrollOffset_=0;
    void publish(){
        constexpr char hex[]="0123456789abcdef";std::string line="FRAME "+std::to_string(address_)+" ";line.reserve(2*pixels_.size()+12);
        const auto offset=scrolling_?static_cast<int>((thinker::clockUs-scrollStarted_)/100000):0;
        for(int page=0;page<HEIGHT/8;++page)for(int x=0;x<WIDTH;++x){uint8_t value=0;for(int bit=0;bit<8;++bit){int y=page*8+bit,sx=x,sy=y;if(scrolling_&&page>=firstPage_&&page<=lastPage_){sx=(x-scrollX_*offset%WIDTH+WIDTH)%WIDTH;sy=(y-scrollY_*offset%HEIGHT+HEIGHT)%HEIGHT;}bool on=presented_[sx+(sy/8)*WIDTH]&(1u<<(sy%8));if(inverted_)on=!on;if(enabled_&&on)value|=1u<<bit;}line+=hex[value>>4];line+=hex[value&15];}
        std::cout<<line<<' '<<(dimmed_?250:1000)<<std::endl;lastScrollOffset_=offset;
    }
    void scroll(int x,int y,int first,int last){if(first<0||last>=HEIGHT/8||first>last)throw std::invalid_argument("Invalid scroll pages");scrolling_=true;scrollX_=x;scrollY_=y;firstPage_=first;lastPage_=last;scrollStarted_=thinker::clockUs;if(begun_)publish();}
};
