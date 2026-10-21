#include "thinkercad/moduledevices.hpp"
#include <algorithm>
namespace thinkercad {
void ModuleDevices::transmitRadio(State& tx,const CircuitPart& part){
    if(!poweredRadios_.contains(part.id))return;
    double frequency=part.model=="cc1101"?double((tx.reg[0x0d]<<16)|(tx.reg[0x0e]<<8)|tx.reg[0x0f])*26e6/65536:double((tx.reg[6]<<16)|(tx.reg[7]<<8)|tx.reg[8])*32e6/524288;
    if(part.model=="cc1101"?!((frequency>=300e6&&frequency<=348e6)||(frequency>=387e6&&frequency<=464e6)||(frequency>=779e6&&frequency<=928e6)):(frequency<137e6||frequency>1020e6))return;
    std::vector<std::uint8_t> payload;
    if(part.model=="cc1101"){
        payload.assign(tx.radioTx.begin(),tx.radioTx.end());tx.radioTx.clear();
        if((tx.reg[8]&3)==1){if(payload.empty()||payload[0]>payload.size()-1)return;payload.resize(payload[0]+1);payload.erase(payload.begin());}
        else if((tx.reg[8]&3)==0){if(payload.size()<tx.reg[6])return;payload.resize(tx.reg[6]);}
        else return;
    }else{for(unsigned i=0;i<tx.reg[0x22];++i)payload.push_back(tx.radioFifo[std::uint8_t(tx.reg[0x0e]+i)]);}
    for(const auto& [id,receiver]:poweredRadios_){
        if(id==part.id||receiver.model!=part.model)continue;
        auto& rx=prepare(receiver,0);
        if(part.model=="cc1101"){
            if(rx.reg[0x35]!=13)continue;
            bool compatible=true;for(int at:{4,5,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10,0x11,0x12,0x13,0x14,0x15})compatible&=tx.reg[at]==rx.reg[at];
            if(!compatible||(rx.reg[8]&7)!=(tx.reg[8]&7)||payload.size()>rx.reg[6])continue;
            unsigned filter=rx.reg[7]&3;if(filter&&(payload.empty()||(payload[0]!=rx.reg[9]&&!(filter>=2&&payload[0]==0)&&!(filter==3&&payload[0]==255))))continue;
            std::vector<std::uint8_t> packet=payload;if((rx.reg[8]&3)==1)packet.insert(packet.begin(),std::uint8_t(payload.size()));if(rx.reg[7]&4){packet.push_back(0x90);packet.push_back(0x80);}
            if(rx.fifo.size()+packet.size()>64){rx.reg[0x35]=17;continue;}rx.fifo.insert(rx.fifo.end(),packet.begin(),packet.end());
        }else{
            if(!(rx.reg[1]&0x80)||((rx.reg[1]&7)!=5&&(rx.reg[1]&7)!=6))continue;
            bool compatible=true;for(int at:{6,7,8,0x1d,0x1e,0x39})compatible&=tx.reg[at]==rx.reg[at];if(!compatible)continue;
            if((rx.reg[0x1d]&1)&&rx.reg[0x22]!=payload.size())continue;
            if(rx.reg[0x12]&0x40)continue; // preserve the unread packet
            for(unsigned i=0;i<payload.size();++i)rx.radioFifo[std::uint8_t(rx.reg[0x0f]+i)]=payload[i];
            rx.reg[0x10]=rx.reg[0x0f];rx.reg[0x13]=std::uint8_t(payload.size());rx.reg[0x12]|=0x50;rx.reg[0x19]=0;rx.reg[0x1a]=100;
            if((rx.reg[1]&7)==6)rx.reg[1]=(rx.reg[1]&0xf8)|1;
        }
    }
}
std::optional<std::uint8_t> ModuleDevices::hardwareSpi(State& s,const CircuitPart& part,std::uint8_t value,Microseconds){
    const auto& id=part.model;if(id!="cc1101"&&id!="sx1276"&&id!="mcp2515")return std::nullopt;
    if(id=="cc1101"){
        auto status=[&](){return std::uint8_t((s.reg[0x35]==13?0x10:s.reg[0x35]==17?0x60:0)|std::min<std::size_t>(15,s.fifo.size()));};
        if(!s.command){
            s.command=true;s.reading=value&0x80;s.burst=value&0x40;s.pointer=value&0x3f;
            if(value>=0x30&&value<=0x3d){
                if(value==0x30)initialize(s,ModuleRegistry::get(id));
                if(value==0x34)s.reg[0x35]=13;
                if(value==0x35){transmitRadio(s,part);s.reg[0x35]=1;}
                if(value==0x36||value==0x39)s.reg[0x35]=1;
                if(value==0x3a){s.fifo.clear();s.reg[0x35]=1;}
                if(value==0x3b)s.radioTx.clear();
            }return status();
        }
        auto at=s.pointer;if(at==0x3f){if(s.reading){if(s.fifo.empty())return 0;auto byte=s.fifo.front();s.fifo.pop_front();return byte;}if(s.radioTx.size()<64)s.radioTx.push_back(value);return status();}
        if(s.burst&&at<0x2f)++s.pointer;
        if(s.reading){if(at==0x3a)return std::uint8_t(s.radioTx.size());if(at==0x3b)return std::uint8_t(s.fifo.size()|(s.reg[0x35]==17?0x80:0));return s.reg[at];}
        if(at<0x2f||at==0x3e)s.reg[at]=value;
        return status();
    }
    if(id=="sx1276"){
        if(!s.command){s.command=true;s.reading=!(value&0x80);s.pointer=value&0x7f;return 0;}
        auto at=s.pointer;if(at!=0)++s.pointer;
        if(at==0){auto& pointer=s.reg[0x0d];if(s.reading)return s.radioFifo[pointer++];s.radioFifo[pointer++]=value;return 0;}
        if(s.reading)return s.reg[at];
        if(at==0x42)return 0;
        if(at==0x12){s.reg[at]&=std::uint8_t(~value);return 0;}
        s.reg[at]=value;if(at==1&&(value&0x80)&&(value&7)==3){transmitRadio(s,part);s.reg[0x12]|=8;s.reg[1]=(value&0xf8)|1;}return 0;
    }
    // MCP2515 command protocol: RESET, READ/WRITE, BIT MODIFY, READ STATUS,
    // LOAD TX BUFFER, RTS and READ RX BUFFER. Loopback uses RXB0 then RXB1.
    auto send=[&](int index){
        int base=0x30+index*0x10;s.reg[base]&=~8;
        if((s.reg[0x0e]&0xe0)==0x40){int rx=!(s.reg[0x2c]&1)?0:!(s.reg[0x2c]&2)?1:-1;
            if(rx>=0){for(int i=1;i<=13;++i)s.reg[0x60+rx*0x10+i]=s.reg[base+i];s.reg[0x2c]|=1<<rx;}else s.reg[0x2d]|=0xc0;
            s.reg[0x2c]|=4<<index;
        }else s.reg[base]|=0x10; // no external CAN controller/ACK model
    };
    if(!s.command){s.command=true;s.opcode=value;s.stage=0;
        if(value==0xc0)initialize(s,ModuleRegistry::get(id));
        if((value&0xf8)==0x80)for(int i=0;i<3;++i)if(value&(1<<i))send(i);
        if((value&0xf8)==0x40)s.pointer=0x31+((value>>1)&3)*0x10+(value&1?5:0);
        if((value&0xfc)==0x90)s.pointer=0x61+((value>>1)&1)*0x10+(value&1?5:0);
        return 0;
    }
    if(s.opcode==0xa0){unsigned out=s.reg[0x2c]&3;for(int i=0;i<3;++i){if(s.reg[0x30+i*16]&8)out|=4<<(i*2);if(s.reg[0x2c]&(4<<i))out|=8<<(i*2);}return std::uint8_t(out);}
    if(s.opcode==2||s.opcode==3||s.opcode==5){
        if(s.stage++==0){s.pointer=value;return 0;}if(s.opcode==3)return s.reg[s.pointer++];
        if(s.opcode==5&&s.stage==2){s.opcode=0x15;s.serialWord=value;return 0;}
    }
    auto store=[&](std::uint8_t at,std::uint8_t byte){s.reg[at]=byte;if(at==0x0f)s.reg[0x0e]=(s.reg[0x0e]&0x1f)|(byte&0xe0);};
    if(s.opcode==2)store(s.pointer++,value);
    else if(s.opcode==0x15){store(s.pointer,(s.reg[s.pointer]&~s.serialWord)|(value&s.serialWord));s.opcode=0;}
    else if((s.opcode&0xf8)==0x40&&((s.opcode>>1)&3)<3)store(s.pointer++,value);
    else if((s.opcode&0xfc)==0x90)return s.reg[s.pointer++];
    return 0;
}
}
