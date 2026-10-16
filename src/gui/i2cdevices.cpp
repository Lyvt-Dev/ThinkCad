#include "i2cdevices.hpp"
#include <algorithm>
#include <stdexcept>
namespace {
// Use the same fixed 5x7 character ROM as the bundled LCD adapter.
#define PROGMEM
#include "../../sdk/gfx/glcdfont.c"
#undef PROGMEM
}
using namespace thinkercad;
I2cDevices::Reply I2cDevices::transfer(const CircuitDocument& doc,const CircuitState& state,const std::string& board,int sda,int scl,int address,const std::vector<uint8_t>& data,int readCount,unsigned clock){
    Reply reply;const auto profile=BoardProfile::get(doc.part(board).model);
    std::vector<const CircuitPart*> matches;
    for(const auto& part:doc.parts()){
        const bool module=part.kind==PartKind::Module;const ModuleDefinition* definition=module?&ModuleRegistry::get(part.model):nullptr;
        if(module?definition->bus!="i2c"||int(parameter(part,"address",definition->address))!=address:((part.kind!=PartKind::I2cCharacterLcd&&part.kind!=PartKind::Oled)||static_cast<int>(part.value)!=address))continue;
        const auto power=module?definition->power:std::string("VCC"),ground=module?definition->ground:std::string("GND");const auto v=state.voltage.find({part.id,power}),g=state.voltage.find({part.id,ground});
        if(v==state.voltage.end()||g==state.voltage.end()||!v->second||!g->second)continue;
        const double supply=*v->second-*g->second;
        if(supply<(module?definition->minVoltage:part.kind==PartKind::Oled?3.:4.5)||supply>(module?definition->maxVoltage:5.5))continue;
        if(!doc.connected({part.id,ground},{board,"GND"})||!doc.connected({part.id,"SDA"},{board,profile.pinName(sda)})||!doc.connected({part.id,"SCL"},{board,profile.pinName(scl)}))continue;
        matches.push_back(&part);
    }
    if(matches.empty())return reply;
    if(matches.size()!=1){reply.status=4;return reply;}
    const auto& part=*matches.front();reply.id=part.id;reply.status=0;
    if(emulation_){for(const auto* pin:{"SDA","SCL"}){const auto value=state.voltage.find({part.id,pin});if(value!=state.voltage.end()&&value->second&&*value->second<.3){reply.status=4;return reply;}}}
    if(part.kind==PartKind::Module){const auto& definition=ModuleRegistry::get(part.model);if(clock>definition.maxClock){reply.status=4;return reply;}reply.bytes=modules_.i2c(part,data,readCount,now_);return reply;}
    auto& device=devices_[part.id];
    if(readCount>=0){
        // SSD1306's serial interface is write-only. PCF8574 reads its port latch.
        if(part.kind==PartKind::Oled)return reply;
        for(int i=0;i<readCount;++i){int value=device.latch;
            if((value&6)==6){const int byte=(value&1)?(device.cgramMode?device.cgram[device.address&63]:device.ddram[device.address&127]):device.address;
                value=(value&15)|((device.readLow?byte<<4:byte)&0xf0);}
            reply.bytes.push_back(static_cast<uint8_t>(value));
        }
    }else if(part.kind==PartKind::Oled)oledWrite(device,data,CircuitDocument::displayHeight(part));
    else for(auto byte:data)lcdWrite(device,byte);
    return reply;
}
void I2cDevices::lcdByte(Device& d,int value,bool rs){
    if(rs){if(d.cgramMode)d.cgram[d.address&63]=value&31;else d.ddram[d.address&127]=value;d.address=(d.address+(d.increment?1:127))&(d.cgramMode?63:127);if(d.entryShift&&!d.cgramMode)d.shift+=(d.increment?1:-1);return;}
    if(value&128){d.address=value&127;d.cgramMode=false;}
    else if(value&64){d.address=value&63;d.cgramMode=true;}
    else if(value&32){d.fourBit=!(value&16);d.nibble=-1;}
    else if(value&16){if(value&8)d.shift+=(value&4)?-1:1;else d.address=(d.address+((value&4)?1:127))&127;}
    else if(value&8){d.visible=value&4;d.cursor=value&2;d.blink=value&1;}
    else if(value&4){d.increment=value&2;d.entryShift=value&1;}
    else if(value&2){d.address=0;d.shift=0;d.cgramMode=false;}
    else if(value&1){d.ddram.fill(' ');d.address=0;d.shift=0;d.cgramMode=false;}
}
void I2cDevices::lcdWrite(Device& d,uint8_t value){
    const int previous=d.latch;d.latch=value;
    // PCF8574 mapping: RS=P0, RW=P1, E=P2, backlight=P3, D4..7=P4..7.
    if(!(previous&4)||(value&4))return;
    if(previous&2){d.readLow=!d.readLow;if(!d.readLow&&(previous&1))d.address=(d.address+(d.increment?1:127))&(d.cgramMode?63:127);return;}
    d.readLow=false;
    const int nibble=(previous>>4)&15;
    if(!d.fourBit){lcdByte(d,nibble<<4,previous&1);return;}
    if(d.nibble<0){d.nibble=nibble;return;}
    const int byte=(d.nibble<<4)|nibble;d.nibble=-1;lcdByte(d,byte,previous&1);
}
void I2cDevices::oledCommand(Device& d,uint8_t value){
    if(d.needed){d.args.push_back(value);if(static_cast<int>(d.args.size())!=d.needed)return;
        switch(d.pending){
        case 0x20:d.mode=d.args[0]%3;break;
        case 0x21:d.columnStart=d.column=d.args[0]&127;d.columnEnd=d.args[1]&127;break;
        case 0x22:d.pageStart=d.page=d.args[0]&7;d.pageEnd=d.args[1]&7;break;
        case 0x81:d.contrast=d.args[0];break;
        case 0xd3:d.offset=d.args[0]&63;break;
        default:break;
        }d.needed=0;d.pending=-1;d.args.clear();return;
    }
    if(value<=0xf)d.column=(d.column&0x70)|value;
    else if(value>=0x10&&value<=0x1f)d.column=(d.column&15)|((value&7)<<4);
    else if(value>=0x40&&value<=0x7f)d.startLine=value&63;
    else if(value>=0xb0&&value<=0xb7)d.page=value&7;
    else switch(value){
    case 0xae:d.visible=false;break;case 0xaf:d.visible=true;break;
    case 0xa4:d.allOn=false;break;case 0xa5:d.allOn=true;break;
    case 0xa6:d.inverted=false;break;case 0xa7:d.inverted=true;break;
    case 0xa0:d.segmentReverse=false;break;case 0xa1:d.segmentReverse=true;break;
    case 0xc0:d.comReverse=false;break;case 0xc8:d.comReverse=true;break;
    case 0x21:case 0x22:d.needed=2;break;
    case 0x20:case 0x81:case 0x8d:case 0xa8:case 0xd3:case 0xd5:case 0xd9:case 0xda:case 0xdb:d.needed=1;break;
    case 0xa3:d.needed=2;break;
    case 0x2e:case 0xe3:break;
    case 0x26:case 0x27:case 0x29:case 0x2a:case 0x2f:throw std::runtime_error("Raw SSD1306 hardware scrolling is not modeled; use the Adafruit_SSD1306 adapter");
    default:throw std::runtime_error("Unsupported raw SSD1306 command");
    }
    if(d.needed){d.pending=value;d.args.clear();}
}
void I2cDevices::oledWrite(Device& d,const std::vector<uint8_t>& data,int height){
    std::size_t i=0;while(i<data.size()){
        const auto control=data[i++];if(control&0x3f)throw std::runtime_error("Invalid SSD1306 I2C control byte");
        const auto end=(control&0x80)?std::min(data.size(),i+1):data.size();
        while(i<end){const auto value=data[i++];if(!(control&0x40)){oledCommand(d,value);continue;}
            d.ram[(d.page&7)*128+(d.column&127)]=value;
            if(d.mode==1){if(++d.page>std::min(d.pageEnd,height/8-1)){d.page=d.pageStart;if(++d.column>d.columnEnd)d.column=d.columnStart;}}
            else if(d.mode==0){if(++d.column>d.columnEnd){d.column=d.columnStart;if(++d.page>std::min(d.pageEnd,height/8-1))d.page=d.pageStart;}}
            else d.column=(d.column+1)&127;
        }
    }
}
Frame I2cDevices::frame(const CircuitPart& part)const{
    const auto& d=devices_.at(part.id);Frame result(128,part.kind==PartKind::Oled?CircuitDocument::displayHeight(part):64);
    if(!d.visible)return result;
    if(part.kind==PartKind::Oled){for(int y=0;y<result.height;++y)for(int x=0;x<128;++x){const int sx=d.segmentReverse?x:127-x;const int sy=((d.comReverse?y:result.height-1-y)+d.startLine-d.offset+64)%64;bool on=d.allOn||((d.ram[(sy/8)*128+sx]>>(sy%8))&1);result.setPixel(x,y,on!=d.inverted);}return result;}
    const int columns=part.model=="lcd-20x4"?20:16,rows=columns==20?4:2;
    const int offsets[]={0,64,20,84};
    for(int row=0;row<rows;++row)for(int col=0;col<columns;++col){const int address=(offsets[row]&64)+((offsets[row]&63)+col+d.shift%40+40)%40;const int c=d.ddram[address];
        for(int x=0;x<5;++x)for(int y=0;y<8;++y)if(c<8?((d.cgram[c*8+y]>>(4-x))&1):((font[c*5+x]>>y)&1))result.setPixel(col*6+x,row*10+y,true);
        if(d.cursor&&address==d.address)for(int x=0;x<5;++x)result.setPixel(col*6+x,row*10+7,true);
    }return result;
}
double I2cDevices::brightness(const CircuitPart& part)const{const auto& d=devices_.at(part.id);return part.kind==PartKind::Oled?double(d.contrast)/255.:(d.latch&8?1.:0.);}
