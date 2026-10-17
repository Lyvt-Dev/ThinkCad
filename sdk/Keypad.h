#pragma once
#include "Arduino.h"
#include <vector>
constexpr char NO_KEY='\0';constexpr int LIST_MAX=10;
enum KeyState {IDLE,PRESSED,HOLD,RELEASED};
using KeypadEvent=char;
struct Key {char kchar=NO_KEY;int kcode=-1;KeyState kstate=IDLE;bool stateChanged=false;};
#define makeKeymap(array) (reinterpret_cast<char*>(array))
class Keypad {
public:
    Key key[LIST_MAX];
    Keypad(char* map,uint8_t* rows,uint8_t* columns,uint8_t rowCount,uint8_t columnCount):map_(map),raw_(rowCount*columnCount),stable_(raw_),changed_(raw_.size()),pressedAt_(raw_.size()){
        if(!map||!rows||!columns||!rowCount||!columnCount||rowCount*columnCount>64)throw std::invalid_argument("Invalid keypad matrix");rows_.assign(rows,rows+rowCount);cols_.assign(columns,columns+columnCount);for(auto pin:rows_)pinMode(pin,INPUT_PULLUP);for(auto pin:cols_)pinMode(pin,INPUT);
    }
    void begin(char* map){if(!map)throw std::invalid_argument("Null keypad map");map_=map;}
    char getKey(){getKeys();for(const auto& k:key)if(k.stateChanged&&k.kstate==PRESSED)return k.kchar;return NO_KEY;}
    bool getKeys(){
        for(auto& k:key){k.stateChanged=false;if(k.kstate==RELEASED)k={};}
        const unsigned long now=millis();if(scanned_&&static_cast<uint32_t>(now-lastScan_)<debounce_)return false;lastScan_=now;scanned_=true;
        std::vector<bool> current(raw_.size());for(std::size_t col=0;col<cols_.size();++col){pinMode(cols_[col],OUTPUT);digitalWrite(cols_[col],LOW);for(std::size_t row=0;row<rows_.size();++row)current[row*cols_.size()+col]=digitalRead(rows_[row])==LOW;pinMode(cols_[col],INPUT);}
        bool any=false;for(std::size_t i=0;i<current.size();++i){if(current[i]!=raw_[i]){raw_[i]=current[i];changed_[i]=now;}
            int slot=findInList(static_cast<int>(i));
            if(stable_[i]!=raw_[i]&&static_cast<uint32_t>(now-changed_[i])>=debounce_){stable_[i]=raw_[i];if(slot<0&&stable_[i])for(int j=0;j<LIST_MAX;++j)if(key[j].kstate==IDLE){slot=j;key[j].kcode=i;key[j].kchar=map_[i];break;}
                if(slot>=0){key[slot].kstate=stable_[i]?PRESSED:RELEASED;key[slot].stateChanged=true;pressedAt_[i]=now;any=true;if(listener_)listener_(key[slot].kchar);}}
            else if(slot>=0&&stable_[i]&&key[slot].kstate==PRESSED&&static_cast<uint32_t>(now-pressedAt_[i])>=hold_){key[slot].kstate=HOLD;key[slot].stateChanged=true;any=true;if(listener_)listener_(key[slot].kchar);}
        }return any;
    }
    char waitForKey(){char c;while((c=getKey())==NO_KEY)delay(10);return c;}
    void setDebounceTime(unsigned time){debounce_=std::max(1u,time);}void setHoldTime(unsigned time){hold_=time;}
    void addEventListener(void (*listener)(char)){listener_=listener;}
    KeyState getState()const{return key[0].kstate;}
    bool isPressed(char c)const{for(const auto& k:key)if(k.kchar==c&&(k.kstate==PRESSED||k.kstate==HOLD))return true;return false;}
    int findInList(char c)const{for(int i=0;i<LIST_MAX;++i)if(key[i].kchar==c&&key[i].kstate!=IDLE)return i;return -1;}
    int findInList(int code)const{for(int i=0;i<LIST_MAX;++i)if(key[i].kcode==code&&key[i].kstate!=IDLE)return i;return -1;}
private:
    char* map_;std::vector<uint8_t> rows_,cols_;std::vector<bool> raw_,stable_;std::vector<unsigned long> changed_,pressedAt_;
    unsigned debounce_=10,hold_=500;unsigned long lastScan_=0;bool scanned_=false;void (*listener_)(char)=nullptr;
};
