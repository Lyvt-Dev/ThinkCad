#pragma once
#include "Arduino.h"
#include <array>
#include <set>
namespace thinkerlcd {
#ifdef FONT5X7_H
#undef FONT5X7_H
#endif
#define font lcdFont
#include "gfx/glcdfont.c"
#undef font
inline int nextToken=10000;
}
constexpr int LCD_5x8DOTS=0,LCD_5x10DOTS=4;
class LiquidCrystal:public Print {
public:
    using Print::write;
    ~LiquidCrystal()override{thinker::timeObservers.erase(this);}
    LiquidCrystal(int rs,int enable,int d4,int d5,int d6,int d7):pins_{rs,enable,d4,d5,d6,d7}{}
    LiquidCrystal(int rs,int rw,int enable,int d4,int d5,int d6,int d7):LiquidCrystal(rs,enable,d4,d5,d6,d7){rw_=rw;}
    void begin(int columns,int rows,int=LCD_5x8DOTS){initialize(columns,rows,false);}
    void clear(){cells_.fill(' ');x_=y_=0;render();}void home(){x_=y_=shift_=0;render();}
    void setCursor(int col,int row){x_=std::clamp(col,0,columns_-1);y_=std::clamp(row,0,rows_-1);render();}
    void display(){visible_=true;render();}void noDisplay(){visible_=false;render();}
    void cursor(){cursor_=true;render();}void noCursor(){cursor_=false;render();}void blink(){blink_=true;render();}void noBlink(){blink_=false;render();}
    void scrollDisplayLeft(){++shift_;render();}void scrollDisplayRight(){--shift_;render();}
    void leftToRight(){direction_=1;}void rightToLeft(){direction_=-1;}void autoscroll(){auto_=true;}void noAutoscroll(){auto_=false;}
    void createChar(std::uint8_t slot,std::uint8_t data[]){if(slot>7)throw std::invalid_argument("LCD glyph index outside 0..7");for(int i=0;i<8;++i)glyphs_[slot][i]=data[i]&31;render();}
    std::size_t write(std::uint8_t c)override{if(!ready_)throw std::runtime_error("Call LCD begin first");if(x_>=0&&x_<columns_&&y_>=0&&y_<rows_)cells_[y_*40+x_]=c;x_+=direction_;if(auto_)shift_+=direction_;render();return 1;}
    std::size_t write(const std::uint8_t* data,std::size_t n)override{if(!ready_)throw std::runtime_error("Call LCD begin first");for(std::size_t i=0;i<n;++i){if(x_>=0&&x_<columns_&&y_>=0&&y_<rows_)cells_[y_*40+x_]=data[i];x_+=direction_;if(auto_)shift_+=direction_;}render();return n;}
    void command(std::uint8_t value){if(value==1)clear();else if(value==2)home();else if(value&128){int address=value&127;const int offsets[]={0,64,20,84};for(int row=rows_-1;row>=0;--row)if(address>=offsets[row]){setCursor(address-offsets[row],row);break;}}else throw std::invalid_argument("Unsupported low-level LCD command");}
protected:
    explicit LiquidCrystal(int address):address_(address){}
    void initialize(int columns,int rows,bool i2c){if(!((columns==16&&rows==2)||(columns==20&&rows==4)))throw std::invalid_argument("LCD supports 16x2 or 20x4");columns_=columns;rows_=rows;cells_.fill(' ');if(i2c){if(address_<8||address_>119)throw std::invalid_argument("Invalid I2C LCD address");std::cout<<"LCD_I2C "<<token_<<' '<<columns<<' '<<rows<<' '<<address_<<std::endl;}else{std::set<int> unique(pins_.begin(),pins_.end());if(unique.size()!=6)throw std::invalid_argument("LCD pins must be distinct");for(int pin:pins_)pinMode(pin,OUTPUT);if(rw_>=0){pinMode(rw_,OUTPUT);digitalWrite(rw_,LOW);}std::cout<<"LCD_BEGIN "<<token_<<' '<<columns<<' '<<rows;for(int pin:pins_)std::cout<<' '<<pin;std::cout<<std::endl;}int ack=1;if(!thinker::standalone&&!(std::cin>>ack))throw std::runtime_error("LCD host disconnected");if(!ack)throw std::runtime_error("LCD not connected: check 5V/GND, RS/E/D4-D7, RW/VO to GND or I2C address");ready_=true;thinker::timeObservers[this]=[this]{auto phase=thinker::clockUs/500000;if(blink_&&phase!=blinkPhase_){blinkPhase_=phase;render();}};render();}
    void setBacklight(bool on){if(ready_)std::cout<<"LCD_LIGHT "<<token_<<' '<<(on?1:0)<<std::endl;}
    int address_=-1,columns_=16,rows_=2;
    void render(){if(!ready_)return;std::array<std::uint8_t,1024> pixels{};auto pixel=[&](int x,int y){if(x>=0&&x<128&&y>=0&&y<64)pixels[x+(y/8)*128]|=1u<<(y%8);};if(visible_)for(int row=0;row<rows_;++row)for(int col=0;col<columns_;++col){int source=(col+shift_)%40;if(source<0)source+=40;auto c=cells_[row*40+source];for(int x=0;x<5;++x)for(int y=0;y<8;++y)if(c<8?((glyphs_[c][y]>>(4-x))&1):((thinkerlcd::lcdFont[c*5+x]>>y)&1))pixel(col*6+x,row*10+y);if(row==y_&&col==x_){if(cursor_)for(int x=0;x<5;++x)pixel(col*6+x,row*10+7);if(blink_&&(blinkPhase_%2))for(int x=0;x<5;++x)for(int y=0;y<8;++y)pixel(col*6+x,row*10+y);}}constexpr char hex[]="0123456789abcdef";std::string encoded;for(auto byte:pixels){encoded+=hex[byte>>4];encoded+=hex[byte&15];}std::cout<<"LCD_FRAME "<<token_<<' '<<encoded<<std::endl;}
private:
    std::array<int,6> pins_{};int rw_=-1,token_=thinkerlcd::nextToken++,x_=0,y_=0,shift_=0,direction_=1;std::uint64_t blinkPhase_=0;bool blink_=false;bool ready_=false,visible_=true,cursor_=false,auto_=false;std::array<std::uint8_t,160> cells_{};std::array<std::array<std::uint8_t,8>,8> glyphs_{};
};
