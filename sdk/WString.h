#pragma once
#include <string>
#include <string_view>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <type_traits>
#include <functional>
class __FlashStringHelper;
class String:public std::string {
    static std::string integer(unsigned long long value,int base,bool negative=false){if(base<2||base>36)base=10;std::string result;do{result.push_back("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"[value%base]);value/=base;}while(value);if(negative)result.push_back('-');std::reverse(result.begin(),result.end());return result;}
    template<class T> static std::string integral(T value,int base){
        if(base<2||base>36)base=10;
        if constexpr(std::is_same_v<T,bool>)return integer(value?1:0,base);
        else {
            using Unsigned=std::make_unsigned_t<T>;
            const bool negative=std::is_signed_v<T>&&value<0&&base==10;
            const auto bits=static_cast<Unsigned>(value);
            const auto magnitude=negative?static_cast<Unsigned>(Unsigned(0)-bits):bits;
            return integer(magnitude,base,negative);
        }
    }
public:
    using std::string::string;using std::string::operator=;
    String()=default;
    String(const std::string& text):std::string(text){}
    String(std::string&& text):std::string(std::move(text)){}
    String(const __FlashStringHelper* text):std::string(text?reinterpret_cast<const char*>(text):""){}
    template<class T> requires(std::is_integral_v<T>&&!std::is_same_v<T,char>)
    explicit String(T value,int base=10):std::string(integral(value,base)){}
    explicit String(char value):std::string(1,value){}
    explicit String(double value,unsigned char decimals=2){std::ostringstream out;out<<std::fixed<<std::setprecision(std::min<unsigned>(decimals,16))<<value;assign(out.str());}
    String& operator=(const __FlashStringHelper* value){assign(value?reinterpret_cast<const char*>(value):"");return *this;}
    unsigned int length()const{return static_cast<unsigned int>(size());}
    char charAt(unsigned int index)const{return index<size()?at(index):0;}
    void setCharAt(unsigned int index,char value){if(index<size())at(index)=value;}
    bool reserve(std::size_t count){try{std::string::reserve(count);return true;}catch(...){return false;}}
    bool concat(const String& value){append(value);return true;}
    bool concat(const char* value){if(!value)return false;append(value);return true;}
    bool concat(const __FlashStringHelper* value){return concat(reinterpret_cast<const char*>(value));}
    bool concat(char value){push_back(value);return true;}
    template<class T> requires(std::is_arithmetic_v<T>) bool concat(T value){append(String(value));return true;}
    bool equals(const String& value)const{return compare(value)==0;}
    bool equalsIgnoreCase(const String& other)const{return size()==other.size()&&std::equal(begin(),end(),other.begin(),[](unsigned char a,unsigned char b){return std::tolower(a)==std::tolower(b);});}
    int compareTo(const String& other)const{return compare(other);}
    bool startsWith(const String& text,unsigned offset=0)const{return offset<=size()&&compare(offset,text.size(),text)==0;}
    bool endsWith(const String& text)const{return size()>=text.size()&&compare(size()-text.size(),text.size(),text)==0;}
    int indexOf(char value,unsigned from=0)const{const auto n=find(value,from);return n==npos?-1:int(n);}
    int indexOf(const String& value,unsigned from=0)const{const auto n=find(value,from);return n==npos?-1:int(n);}
    int lastIndexOf(char value,unsigned from=~0u)const{const auto n=rfind(value,from);return n==npos?-1:int(n);}
    int lastIndexOf(const String& value,unsigned from=~0u)const{const auto n=rfind(value,from);return n==npos?-1:int(n);}
    String substring(unsigned first,unsigned last=~0u)const{if(first>last)std::swap(first,last);first=std::min<unsigned>(first,length());last=std::min<unsigned>(last,length());return substr(first,last-first);}
    void remove(unsigned first,unsigned count=~0u){if(first<size())erase(first,std::min<std::size_t>(count,size()-first));}
    void replace(char from,char to){std::replace(begin(),end(),from,to);}
    void replace(const String& from,const String& to){if(from.empty())return;std::size_t at=0;while((at=find(from,at))!=npos){std::string::replace(at,from.size(),to);at+=to.size();}}
    void trim(){auto white=[](unsigned char c){return std::isspace(c)!=0;};auto first=std::find_if_not(begin(),end(),white);erase(begin(),first);auto last=std::find_if_not(rbegin(),rend(),white).base();erase(last,end());}
    void toLowerCase(){for(auto& c:*this)c=char(std::tolower(static_cast<unsigned char>(c)));}
    void toUpperCase(){for(auto& c:*this)c=char(std::toupper(static_cast<unsigned char>(c)));}
    long toInt()const{return std::strtol(c_str(),nullptr,10);}
    float toFloat()const{return std::strtof(c_str(),nullptr);}
    double toDouble()const{return std::strtod(c_str(),nullptr);}
    void toCharArray(char* out,unsigned count,unsigned from=0)const{if(!out||!count)return;const auto n=from<size()?std::min<std::size_t>(count-1,size()-from):0;if(n)copy(out,n,from);out[n]=0;}
    void getBytes(unsigned char* out,unsigned count,unsigned from=0)const{toCharArray(reinterpret_cast<char*>(out),count,from);}
    explicit operator bool()const{return !empty();}
};
namespace std {template<> struct hash<String>{size_t operator()(const String& value)const noexcept{return hash<std::string>{}(value);}};}
