#include "HardwareSerial.h"
#include <sstream>
#include <iostream>
#include <stdexcept>
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
int main(){std::ostringstream output;std::istringstream replies("1 16 17 2 65 65 2 0 1 18 19 0\n");auto* originalOut=std::cout.rdbuf(output.rdbuf());auto* originalIn=std::cin.rdbuf(replies.rdbuf());try{
    HardwareSerial serial(2);require(serial.read()==-1&&!serial,"uninitialized UART is inactive");serial.begin(9600,SERIAL_8N1,16,17);require(bool(serial)&&serial.getRxPin()==16&&serial.getTxPin()==17,"host resolves actual pin assignment");require(serial.available()==2&&serial.peek()==65&&serial.read()==65,"Stream receive calls use wired UART channel");require(serial.write("AB",2)==2,"Print buffer writes a bounded UART packet");serial.end();require(!serial&&thinker::modes[17]==INPUT,"ending UART releases native pin state");HardwareSerial other(1);require(other.setPins(18,19),"pins can be chosen before begin");other.begin(115200);other.end();require(output.str().find("UART_BEGIN 2 9600 6 16 17 0")!=std::string::npos&&output.str().find("UART_WRITE 2 4142")!=std::string::npos&&output.str().find("UART_BEGIN 1 115200 6 18 19 0")!=std::string::npos,"native protocol retains channel, format and requested pins");
    std::cout.rdbuf(originalOut);std::cin.rdbuf(originalIn);std::cout<<"HardwareSerial native configuration, Stream, pin and packet API checks passed\n";return 0;
}catch(const std::exception& e){std::cout.rdbuf(originalOut);std::cin.rdbuf(originalIn);std::cerr<<e.what()<<'\n';return 1;}}
