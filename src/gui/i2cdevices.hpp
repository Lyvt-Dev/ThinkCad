#pragma once
#include "thinkercad/circuit.hpp"
#include "thinkercad/moduledevices.hpp"
#include <array>
#include <vector>

// Register-level models shared by all masters addressing the same physical part.
class I2cDevices {
public:
    struct Reply {int status=2;std::vector<uint8_t> bytes;std::string id;};
    void reset(){devices_.clear();modules_.reset();}
    void context(thinkercad::Microseconds now,bool emulation){now_=now;emulation_=emulation;modules_.setEmulation(emulation);}
    thinkercad::ModuleDevices& modules(){return modules_;}
    Reply transfer(const thinkercad::CircuitDocument&,const thinkercad::CircuitState&,
                   const std::string& board,int sda,int scl,int address,
                   const std::vector<uint8_t>& data,int readCount=-1,unsigned clock=400000);
    thinkercad::Frame frame(const thinkercad::CircuitPart&)const;
    double brightness(const thinkercad::CircuitPart&)const;
private:
    thinkercad::ModuleDevices modules_;
    thinkercad::Microseconds now_=0;
    bool emulation_=false;
    struct Device {
        Device(){ddram.fill(' ');}
        std::array<uint8_t,128> ddram{};std::array<uint8_t,64> cgram{};
        std::array<uint8_t,1024> ram{};
        int latch=255,address=0,shift=0,nibble=-1;
        bool readLow=false,fourBit=false,cgramMode=false,increment=true,entryShift=false,visible=false,cursor=false,blink=false;
        int column=0,page=0,columnStart=0,columnEnd=127,pageStart=0,pageEnd=7,mode=2,pending=-1,needed=0;
        std::vector<uint8_t> args;
        bool inverted=false,allOn=false,segmentReverse=false,comReverse=false;
        int startLine=0,offset=0,contrast=127;
    };
    std::map<std::string,Device> devices_;
    static void lcdByte(Device&,int,bool);
    static void lcdWrite(Device&,uint8_t);
    static void oledCommand(Device&,uint8_t);
    static void oledWrite(Device&,const std::vector<uint8_t>&,int);
};
