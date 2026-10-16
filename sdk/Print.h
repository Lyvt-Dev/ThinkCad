#pragma once
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <string>
#include <sstream>
#include <iomanip>
#include <cstdarg>
#include <cstdio>
#include <vector>
constexpr int DEC=10, HEX=16, OCT=8, BIN=2;
class __FlashStringHelper;
using String=std::string;
class Print {
public:
    virtual ~Print()=default;
    virtual std::size_t write(std::uint8_t)=0;
    std::size_t write(int value){return write(static_cast<std::uint8_t>(value));}
    std::size_t write(unsigned value){return write(static_cast<std::uint8_t>(value));}
    std::size_t write(long value){return write(static_cast<std::uint8_t>(value));}
    std::size_t write(unsigned long value){return write(static_cast<std::uint8_t>(value));}
    virtual std::size_t write(const std::uint8_t* data,std::size_t size){if(!data&&size){setWriteError();return 0;}std::size_t n=0;while(n<size&&write(data[n]))++n;return n;}
    std::size_t write(const char* data,std::size_t size){return write(reinterpret_cast<const std::uint8_t*>(data),size);}
    std::size_t write(const char* text){return text?write(reinterpret_cast<const std::uint8_t*>(text),std::strlen(text)):0;}
    std::size_t print(const char* text){return write(text);}
    std::size_t print(const String& text){return write(reinterpret_cast<const std::uint8_t*>(text.data()),text.size());}
    std::size_t print(const __FlashStringHelper* text){return write(reinterpret_cast<const char*>(text));}
    std::size_t print(char c){return write(static_cast<std::uint8_t>(c));}
    std::size_t print(unsigned char n,int base=DEC){return number(n,base);}
    std::size_t print(int n,int base=DEC){return print(static_cast<long long>(n),base);}
    std::size_t print(unsigned n,int base=DEC){return number(n,base);}
    std::size_t print(long n,int base=DEC){return print(static_cast<long long>(n),base);}
    std::size_t print(unsigned long n,int base=DEC){return number(n,base);}
    std::size_t print(long long n,int base=DEC){if(base==DEC&&n<0)return write('-')+number(0-static_cast<unsigned long long>(n),base);return number(static_cast<unsigned long long>(n),base);}
    std::size_t print(unsigned long long n,int base=DEC){return number(n,base);}
    std::size_t print(double n,int digits=2){std::ostringstream out;out<<std::fixed<<std::setprecision(digits<0?0:digits>20?20:digits)<<n;return print(out.str());}
    std::size_t println(){return write("\r\n");}
    template<class T>std::size_t println(const T& value){return print(value)+println();}
    template<class T>std::size_t println(const T& value,int option){return print(value,option)+println();}
    int printf(const char* format,...){va_list args;va_start(args,format);va_list copy;va_copy(copy,args);int size=std::vsnprintf(nullptr,0,format,copy);va_end(copy);if(size<0||size>1048576){va_end(args);return 0;}std::vector<char> text(size+1);std::vsnprintf(text.data(),text.size(),format,args);va_end(args);return static_cast<int>(write(reinterpret_cast<const std::uint8_t*>(text.data()),size));}
    int getWriteError()const{return error_;}void clearWriteError(){error_=0;}virtual int availableForWrite(){return 0;}virtual void flush(){}
protected:void setWriteError(int error=1){error_=error;}
private:
    int error_=0;
    std::size_t number(unsigned long long n,int base){if(base==0)return write(static_cast<std::uint8_t>(n));if(base<2||base>36)base=10;char data[65];int i=65;do{auto digit=n%base;data[--i]=static_cast<char>(digit<10?'0'+digit:'A'+digit-10);n/=base;}while(n);return write(reinterpret_cast<const std::uint8_t*>(data+i),65-i);}
};
