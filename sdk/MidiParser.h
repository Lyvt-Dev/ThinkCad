#pragma once
#include <cstdint>
#include <vector>
namespace thinkermidi {
struct Message {uint8_t status=0,data1=0,data2=0;std::vector<uint8_t> sysex;};
class Parser {
public:
    bool feed(uint8_t b,Message& out){
        if(b>=0xf8){out={b,0,0,{}};return true;}
        if(b&0x80){if(b==0xf7&&exclusive_){exclusive_=false;if(overflow_){buffer_.clear();return false;}buffer_.push_back(b);out={0xf0,0,0,buffer_};buffer_.clear();return true;}
            exclusive_=false;buffer_.clear();overflow_=false;
            if(b==0xf0){running_=0;status_=0;exclusive_=true;buffer_.push_back(b);return false;}
            status_=b;running_=b<0xf0?b:0;needed_=length(b);
            if(!needed_){if(b==0xf6){out={b,0,0,{}};return true;}status_=0;}return false;
        }
        if(exclusive_){if(buffer_.size()<1023)buffer_.push_back(b);else overflow_=true;return false;}
        if(!status_){status_=running_;needed_=length(status_);if(!status_)return false;}
        buffer_.push_back(b);if(int(buffer_.size())<needed_)return false;
        out={status_,buffer_[0],needed_>1?buffer_[1]:uint8_t(0),{}};buffer_.clear();status_=running_;needed_=length(status_);return true;
    }
private:
    uint8_t status_=0,running_=0;int needed_=0;bool exclusive_=false,overflow_=false;std::vector<uint8_t> buffer_;
    static int length(uint8_t s){if(s>=0x80&&s<0xf0)return (s&0xf0)==0xc0||(s&0xf0)==0xd0?1:2;return s==0xf2?2:(s==0xf1||s==0xf3?1:0);}
};
}
