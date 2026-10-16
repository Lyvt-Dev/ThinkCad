#pragma once
#include "Print.h"
#include <algorithm>
#include <chrono>
#include <thread>
#include <cctype>
#include <cstdlib>

enum LookaheadMode {SKIP_ALL,SKIP_NONE,SKIP_WHITESPACE};
constexpr char NO_IGNORE_CHAR='\x01';
// Common Arduino Stream helpers, with timeouts measured in simulated time.
class Stream : public Print {
public:
    virtual int available()=0;
    virtual int read()=0;
    virtual int peek()=0;
    void setTimeout(unsigned long milliseconds){timeout_=milliseconds;}
    unsigned long getTimeout()const{return timeout_;}
    std::size_t readBytes(char* data,std::size_t length){return readBytes(reinterpret_cast<uint8_t*>(data),length);}
    std::size_t readBytes(uint8_t* data,std::size_t length){if(!data)return 0;std::size_t n=0;while(n<length){int c=timedRead();if(c<0)break;data[n++]=static_cast<uint8_t>(c);}return n;}
    std::size_t readBytesUntil(char terminator,char* data,std::size_t length){return readBytesUntil(terminator,reinterpret_cast<uint8_t*>(data),length);}
    std::size_t readBytesUntil(char terminator,uint8_t* data,std::size_t length){if(!data)return 0;std::size_t n=0;while(n<length){int c=timedRead();if(c<0||c==static_cast<unsigned char>(terminator))break;data[n++]=static_cast<uint8_t>(c);}return n;}
    String readString(){String text;int c;while((c=timedRead())>=0)text+=static_cast<char>(c);return text;}
    String readStringUntil(char terminator){String text;int c;while((c=timedRead())>=0&&c!=static_cast<unsigned char>(terminator))text+=static_cast<char>(c);return text;}
    long parseInt(LookaheadMode mode=SKIP_ALL,char ignore=NO_IGNORE_CHAR){return static_cast<long>(parseNumber(false,mode,ignore));}
    float parseFloat(LookaheadMode mode=SKIP_ALL,char ignore=NO_IGNORE_CHAR){return static_cast<float>(parseNumber(true,mode,ignore));}
    bool find(const char* target){return target&&findUntil(target,std::strlen(target),nullptr,0);}
    bool find(char* target){return find(static_cast<const char*>(target));}
    bool find(const uint8_t* target){return find(reinterpret_cast<const char*>(target));}
    bool find(char target){return findUntil(&target,1,nullptr,0);}
    bool find(const char* target,std::size_t length){return findUntil(target,length,nullptr,0);}
    bool find(const uint8_t* target,std::size_t length){return find(reinterpret_cast<const char*>(target),length);}
    bool findUntil(const char* target,const char* terminator){return target&&findUntil(target,std::strlen(target),terminator,terminator?std::strlen(terminator):0);}
    bool findUntil(const char* target,std::size_t length,const char* terminator,std::size_t stopLength){
        if(!target)return false;if(!length)return true;
        String seen;const std::size_t limit=std::max(length,stopLength);int c;
        while((c=timedRead())>=0){seen+=static_cast<char>(c);if(seen.size()>limit)seen.erase(0,seen.size()-limit);
            if(seen.size()>=length&&seen.compare(seen.size()-length,length,target,length)==0)return true;
            if(terminator&&stopLength&&seen.size()>=stopLength&&seen.compare(seen.size()-stopLength,stopLength,terminator,stopLength)==0)return false;
        }return false;
    }
protected:
    virtual uint64_t streamClock()const{return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();}
    virtual void streamWait(uint64_t us){std::this_thread::sleep_for(std::chrono::microseconds(us));}
    int timedRead(){return timed(false);}
    int timedPeek(){return timed(true);}
private:
    unsigned long timeout_=1000;
    int timed(bool peeking){const auto start=streamClock();int c;do{c=peeking?peek():read();if(c>=0)return c;if(streamClock()-start>=uint64_t(timeout_)*1000)return -1;streamWait(std::min<uint64_t>(1000,uint64_t(timeout_)*1000-(streamClock()-start)));}while(true);}
    double parseNumber(bool floating,LookaheadMode mode,char ignore){
        int c;while((c=timedPeek())>=0){if(std::isdigit(c)||c=='-'||(floating&&c=='.'))break;
            if(mode==SKIP_NONE||(mode==SKIP_WHITESPACE&&!std::isspace(c)))return 0;read();}
        String number;bool dot=false;
        while((c=timedPeek())>=0){if(c==ignore){read();continue;}if(c=='-'&&number.empty()){number+='-';read();continue;}
            if(std::isdigit(c)){number+=static_cast<char>(read());continue;}if(floating&&c=='.'&&!dot){dot=true;number+='.';read();continue;}break;}
        return number.empty()?0:std::strtod(number.c_str(),nullptr);
    }
};
