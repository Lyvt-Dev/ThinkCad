#include "Wire.h"
#include <sstream>
#include <stdexcept>
#include <iostream>
void check(bool value,const char* text){if(!value)throw std::runtime_error(text);}
class MemoryStream:public Stream {
public:
    explicit MemoryStream(String text):text_(std::move(text)){setTimeout(0);}
    int available()override{return static_cast<int>(text_.size()-at_);}
    int read()override{return available()?static_cast<unsigned char>(text_[at_++]):-1;}
    int peek()override{return available()?static_cast<unsigned char>(text_[at_]):-1;}
    std::size_t write(uint8_t)override{return 0;}
private:String text_;std::size_t at_=0;
};
int main(){auto* out=std::cout.rdbuf();auto* in=std::cin.rdbuf();try{
    std::ostringstream messages;std::istringstream replies("0 1000 2 1000 3 1000 3 104 105 33 1000 0 1000 1 8 1000");
    std::cout.rdbuf(messages.rdbuf());std::cin.rdbuf(replies.rdbuf());
    Wire.begin();check(Wire.sda()==SDA&&Wire.scl()==SCL,"board default bus pins");
    Wire.beginTransmission(0x27);check(Wire.write("hello world")==11,"string write");check(Wire.endTransmission()==0,"ACK");
    check(messages.str().find("I2C_WRITE 39 1 100000 68656c6c6f20776f726c64")!=std::string::npos,"actual payload delivered to host");
    Wire.beginTransmission(0x28);check(Wire.endTransmission(false)==2,"address NACK");
    Wire.beginTransmission(0x27);Wire.write(1);check(Wire.endTransmission()==3,"data NACK");
    check(Wire.requestFrom(0x27,3)==3,"read count");check(Wire.peek()==104&&Wire.available()==3,"peek does not consume");char data[4]{};check(Wire.readBytes(data,2)==2&&std::string(data)=="hi","buffer read");check(Wire.read()==33&&Wire.read()==-1&&Wire.peek()==-1,"read exhaustion");
    check(Wire.requestFrom(uint8_t(0x27),uint8_t(1),uint32_t(0x1234),uint8_t(2))==1&&Wire.read()==8,"internal register repeated start read");
    check(messages.str().find("I2C_WRITE 39 0 100000 1234")!=std::string::npos,"repeated START and big endian register");
    Wire.beginTransmission(0x27);for(std::size_t i=0;i<TwoWire::bufferLength;++i)check(Wire.write(uint8_t(i))==1,"buffer capacity");check(Wire.write(0)==0&&Wire.getWriteError()&&Wire.endTransmission()==1,"overflow reported without transaction");
    Wire.setClock(400000);check(Wire.getClock()==400000,"clock");Wire.end();bool rejected=false;try{Wire.beginTransmission(0x27);}catch(const std::runtime_error&){rejected=true;}check(rejected,"use after end rejected");
    Wire.begin(2,3);Wire.begin();check(Wire.sda()==2&&Wire.scl()==3,"library begin preserves explicitly selected pins");Wire.end();
    MemoryStream numbers("value=-123,45.25;done");check(numbers.parseInt()==-123&&numbers.read()==','&&numbers.parseFloat()==45.25f,"Stream number parsing and delimiters");check(numbers.readStringUntil(';').empty()&&numbers.readString()=="done","Stream strings and exhaustion");
    MemoryStream search("abababacENDrest");check(search.find("ababac")&&search.findUntil("rest","END")==false,"Stream overlapping find and terminator");
    MemoryStream lines("abc\nrest");char line[8]{};check(lines.readBytesUntil('\n',line,7)==3&&std::string(line)=="abc"&&lines.peek()=='r',"Stream delimiter buffer read");
    Stream* stream=&Wire;check(stream->available()==0&&stream->read()==-1,"Wire is an Arduino Stream");
    std::cout.rdbuf(out);std::cin.rdbuf(in);std::cout<<"Wire transactions, string/buffer writes, ACK/NACK, repeated START, reads and overflow passed\n";return 0;
}catch(const std::exception& e){std::cout.rdbuf(out);std::cin.rdbuf(in);std::cerr<<e.what()<<'\n';return 1;}}
