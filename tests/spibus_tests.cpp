#include "thinkercad/spibus.hpp"
#include <iostream>
#include <stdexcept>
using namespace thinkercad;
namespace {
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
struct Fixture {
    CircuitDocument document;
    Esp8266Board board;
    ModuleDevices devices;
    SpiBus bus{devices};
    Fixture(){board.pinMode(14,PinMode::Output);board.pinMode(13,PinMode::Output);board.pinMode(12,PinMode::Input);board.pinMode(15,PinMode::Output);board.digitalWrite(15,true);}
    std::string add(bool power=true,bool ground=true,bool mosi=true){
        const auto id=document.addPart(PartKind::Module,200,0);document.part(id).model="bme280-spi";
        if(power)document.connect({"board","3V3"},{id,"VCC"});
        if(ground)document.connect({"board","GND"},{id,"GND"});
        document.connect({"board","GPIO14"},{id,"SCK"});document.connect({"board","GPIO12"},{id,"MISO"});
        if(mosi)document.connect({"board","GPIO13"},{id,"MOSI"});
        document.connect({"board","GPIO15"},{id,"CS"});return id;
    }
    SpiBus::Reply byte(unsigned value,unsigned clock=1000000,int mode=0,bool lsb=false){return bus.transfer(document,document.solve(board),"board",14,12,13,clock,mode,lsb,value,1000);}
    void select(bool active){board.digitalWrite(15,!active);bus.refreshSelects(document,document.solve(board));}
};
}
int main(){
    for(int missing=0;missing<3;++missing){Fixture f;f.add(missing!=0,missing!=1,missing!=2);f.select(true);require(f.byte(0xd0).status==SpiBus::Status::Unselected,"unwired or unpowered device must not respond");}
    Fixture f;const auto id=f.add();require(f.byte(0xd0).status==SpiBus::Status::Unselected,"CS defaults inactive");f.select(true);require(f.byte(0xd0).status==SpiBus::Status::Ok,"wired selected device acknowledges");const auto value=f.byte(0);require(value.id==id&&value.value==0x60,"physical SPI bus reads device identity");
    f.select(false);f.select(true);require(f.byte(0xd0,1000000,1).status==SpiBus::Status::Mode,"unsupported clock phase rejected");require(f.byte(0xd0,80000000).status==SpiBus::Status::Clock,"excessive SPI frequency rejected");
    f.select(false);f.select(true);f.byte(0xd0,1000000,3);require(f.byte(0,1000000,3).value==0x60,"BME280 also supports mode 3");
    f.select(false);f.select(true);f.byte(0x0b,1000000,0,true);require(f.byte(0,1000000,0,true).value==6,"LSB first changes bits on physical bus");
    f.add();require(f.byte(0xd0).status==SpiBus::Status::Collision,"two asserted CS lines cause collision");
    std::cout<<"Physical SPI wiring, supply, selection, clock, bit ordering and collision checks passed\n";
}
