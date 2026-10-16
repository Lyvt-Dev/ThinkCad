#include "thinkercad/uartbus.hpp"
#include <iostream>
#include <stdexcept>
#include <algorithm>
using namespace thinkercad;
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
struct Fixture {
    CircuitDocument doc;Esp8266Board a,b;std::string second;UartBus bus;
    Fixture(bool ground=true){doc.part("board").model="esp32";second=doc.addPart(PartKind::Esp8266,400,0);doc.part(second).model="esp32";for(auto* board:{&a,&b}){board->configure("esp32");board->pinMode(16,PinMode::Input);board->pinMode(17,PinMode::Output);board->digitalWrite(17,true);}doc.connect({"board","GPIO17"},{second,"GPIO16"});doc.connect({second,"GPIO17"},{"board","GPIO16"});if(ground)doc.connect({"board","GND"},{second,"GND"});configure();}
    UartBus::Config settings()const{UartBus::Config c;c.baud=9600;c.rx=16;c.tx=17;return c;}
    void configure(){bus.begin(doc,"board",1,settings());bus.begin(doc,second,1,settings());}
    CircuitState state()const{return doc.solve({{"board",&a},{second,&b}});}
};
int main(){
    for(const auto& name:BoardProfile::catalog()){const auto profile=BoardProfile::get(name);for(int port:UartBus::supportedPorts(profile)){const auto [rx,tx]=UartBus::defaultPins(profile,port);require(rx<0||std::find(profile.gpio.begin(),profile.gpio.end(),rx)!=profile.gpio.end(),"default UART RX is an exposed GPIO");require(tx<0||std::find(profile.gpio.begin(),profile.gpio.end(),tx)!=profile.gpio.end(),"default UART TX is an exposed GPIO");}}
    Fixture f;f.bus.write(f.doc,f.state(),"board",1,{'H','i'},100,true);require(f.bus.available(f.second,1,1141)==0&&f.bus.available(f.second,1,1142)==1,"8N1 byte arrives after ten baud periods");require(f.bus.available(f.second,1,2184)==2,"transmissions serialize on a port");require(f.bus.read(f.second,1,2184,true)=='H'&&f.bus.read(f.second,1,2184)=='H'&&f.bus.read(f.second,1,2184)=='i',"peek preserves RX FIFO order");require(f.bus.read(f.second,1,2184)==-1,"empty UART returns -1");f.bus.write(f.doc,f.state(),f.second,1,{'R'},3000,true);require(f.bus.read("board",1,4042)=='R',"crossed wires provide full duplex");
    auto wrong=f.settings();wrong.baud=115200;f.bus.begin(f.doc,f.second,1,wrong);f.bus.write(f.doc,f.state(),"board",1,{'x'},10000,true);require(f.bus.available(f.second,1,100000)==0&&f.bus.errors(f.second,1).framing==1,"baud mismatch causes framing error");wrong=f.settings();wrong.parity=UartBus::Parity::Odd;f.bus.begin(f.doc,f.second,1,wrong);f.bus.write(f.doc,f.state(),"board",1,{'x'},100000,true);require(f.bus.errors(f.second,1).framing==1,"parity mismatch is rejected");
    auto five=f.settings();five.dataBits=5;f.bus.begin(f.doc,"board",1,five);f.bus.begin(f.doc,f.second,1,five);f.bus.write(f.doc,f.state(),"board",1,{255},200000,false);require(f.bus.read(f.second,1,200000)==31,"configured data width masks transferred byte");
    f.configure();std::vector<std::uint8_t> bytes(300);for(unsigned i=0;i<bytes.size();++i)bytes[i]=std::uint8_t(i);f.bus.write(f.doc,f.state(),"board",1,bytes,0,true);require(f.bus.available(f.second,1,1000000)==256&&f.bus.errors(f.second,1).overrun==44,"RX memory is bounded with explicit overrun count");for(int i=0;i<256;++i)require(f.bus.read(f.second,1,1000000)==i,"overrun preserves oldest unread bytes");
    const auto third=f.doc.addPart(PartKind::Esp8266,700,0);f.doc.part(third).model="esp32";Esp8266Board c;c.configure("esp32");c.pinMode(17,PinMode::Output);c.digitalWrite(17,true);f.doc.connect({third,"GPIO17"},{f.second,"GPIO16"});f.doc.connect({third,"GND"},{"board","GND"});f.bus.begin(f.doc,third,1,f.settings());const auto state=f.doc.solve({{"board",&f.a},{f.second,&f.b},{third,&c}});f.bus.write(f.doc,state,"board",1,{'A'},1000000,true);f.bus.write(f.doc,state,third,1,{'C'},1000000,true);require(f.bus.available(f.second,1,2000000)==0&&f.bus.errors(f.second,1).collision==2,"overlapping transmitters corrupt both frames");
    Fixture floating(false);floating.bus.write(floating.doc,floating.state(),"board",1,{'x'},0,false);require(floating.bus.available(floating.second,1,10000)==0,"UART requires actual common-ground wiring");bool rejected=false;try{f.bus.begin(f.doc,"board",2,f.settings());}catch(const std::invalid_argument&){rejected=true;}require(rejected,"two UARTs cannot own the same GPIO pins");
    std::cout<<"UART wiring, virtual byte timing, full duplex, formats, bounded FIFO and collision checks passed\n";
}
