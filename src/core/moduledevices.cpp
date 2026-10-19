#include "thinkercad/moduledevices.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
namespace thinkercad {
void ModuleDevices::synchronizePower(const CircuitDocument& doc,const CircuitState& state){
    std::set<std::string> present;
    for(const auto& part:doc.parts())if(part.kind==PartKind::Module){present.insert(part.id);const auto& d=ModuleRegistry::get(part.model);auto voltage=[&](const std::string& pin)->std::optional<double>{auto it=state.voltage.find({part.id,pin});return it==state.voltage.end()?std::nullopt:it->second;};const auto v=voltage(d.power),g=voltage(d.ground);bool ready=v&&g&&*v-*g>=d.minVoltage&&*v-*g<=d.maxVoltage;if(ready&&(d.implementation=="mfrc522"||d.implementation=="vs1053b")){const auto reset=voltage(d.implementation=="vs1053b"?"RESET":"RST");ready=reset&&*reset-*g>=.7*(*v-*g);}if(!ready)states_.erase(part.id);}
    std::erase_if(states_,[&](const auto& entry){return !present.contains(entry.first);});
}
namespace {
using Registers=std::array<std::uint8_t,256>;
void word(Registers& reg,int at,int value,bool little=false){const auto bits=std::uint16_t(value);reg[at]=std::uint8_t(little?bits:bits>>8);reg[at+1]=std::uint8_t(little?bits>>8:bits);}
int searchRaw(int max,double target,const std::function<double(int)>& convert,bool ascending=true){int lo=0,hi=max;while(lo<hi){const int mid=lo+(hi-lo)/2;const bool below=convert(mid)<target;if(below==ascending)lo=mid+1;else hi=mid;}return lo;}
double fineTemperature(int raw){const double a=(raw/16384.-27504./1024.)*26435.;const double b=raw/131072.-27504./8192.;return a+b*b*-1000.;}
double pressure(int raw,double fine){double v1=fine/2.-64000.;double v2=v1*v1*-7./32768.+v1*140.*2.;v2=v2/4.+2855.*65536.;v1=(3024.*v1*v1/524288.+-10685.*v1)/524288.;v1=(1.+v1/32768.)*36477.;double p=(1048576.-raw-v2/4096.)*6250./v1;v1=6000.*p*p/2147483648.;v2=p*-14600./32768.;return p+(v1+v2+15500.)/16.;}
double humidity(int raw,double fine){double h=fine-76800.;h=(raw-(325.*64.+50./16384.*h))*(362./65536.*(1.+30./67108864.*h*(1.+0./67108864.*h)));h*=1.-75.*h/524288.;return std::clamp(h,0.,100.);}
std::array<std::uint8_t,2> crcA(const std::vector<std::uint8_t>& data){unsigned crc=0x6363;for(auto byte:data){unsigned value=byte^(crc&255);value^=(value<<4)&255;crc=(crc>>8)^(value<<8)^(value<<3)^(value>>4);}return {std::uint8_t(crc),std::uint8_t(crc>>8)};}
}
void ModuleDevices::initialize(State& state,const ModuleDefinition& d){
    const bool selected=state.selected,dataMode=state.dataMode;state=State{};state.generation=++generationCounter_;state.dataMode=dataMode;state.initialized=true;state.model=d.id;state.selected=selected;
    for(const auto& [reg,value]:d.registers)state.reg[reg]=std::uint8_t(value);
    if(d.implementation=="wm8731"){state.reg[12]=0x9f;}if(d.implementation=="cs4272"){state.reg[7]=1;state.reg[8]=0xc0;}
    state.midiVolume.fill(127);state.midiPan.fill(64);
    if(d.implementation=="vs1053b"){state.reg[1]=0x08;state.reg[2]=0x40;state.reg[10]=0x45;state.reg[11]=0xac;}
    if(d.implementation=="mpr121"){state.reg[0x5c]=0x10;state.reg[0x5d]=0x24;}
    if(d.implementation=="bme280"){
        auto& r=state.reg;r[0xd0]=0x60;const int calib[]={27504,26435,-1000,36477,-10685,3024,2855,140,-7,15500,-14600,6000};for(int i=0;i<12;++i)word(r,0x88+i*2,calib[i],true);r[0xa1]=75;word(r,0xe1,362,true);r[0xe3]=0;r[0xe4]=325>>4;r[0xe5]=(325&15)|((50&15)<<4);r[0xe6]=50>>4;r[0xe7]=30;r[0xf7]=r[0xfa]=r[0xfd]=0x80;
    }else if(d.implementation=="mpu6050"){state.reg[0x75]=0x68;state.reg[0x6b]=0x40;}
    else if(d.implementation=="mfrc522"){state.reg[0x37]=0x92;state.reg[0x11]=0x3f;}
}
void ModuleDevices::sampleBme(State& state,const CircuitPart& part){
    auto& r=state.reg;const int t=searchRaw(0xfffff,parameter(part,"temperature",25),[](int raw){return fineTemperature(raw)/5120.;});const double fine=fineTemperature(t);const int p=searchRaw(0xfffff,parameter(part,"pressure",1013.25)*100,[&](int raw){return pressure(raw,fine);},false);const int h=searchRaw(0xffff,parameter(part,"humidity",50),[&](int raw){return humidity(raw,fine);});
    auto twenty=[&](int at,int value){r[at]=std::uint8_t(value>>12);r[at+1]=std::uint8_t(value>>4);r[at+2]=std::uint8_t(value<<4);};if((r[0xf4]>>2)&7)twenty(0xf7,p);if((r[0xf4]>>5)&7)twenty(0xfa,t);if(r[0xf2]&7)word(r,0xfd,h);
}
void ModuleDevices::sampleMpu(State& state,const CircuitPart& part){
    auto& r=state.reg;const double accel=16384./(1<<((r[0x1c]>>3)&3));const double gyro[]={131.,65.5,32.8,16.4};const char* axes[]={"accelX","accelY","accelZ","gyroX","gyroY","gyroZ"};
    for(int i=0;i<6;++i){const auto value=std::clamp(std::llround(parameter(part,axes[i],i==2?1:0)*(i<3?accel:gyro[(r[0x1b]>>3)&3])),-32768ll,32767ll);word(r,i<3?0x3b+i*2:0x43+(i-3)*2,int(value));}word(r,0x41,int(std::clamp(std::llround((parameter(part,"temperature",25)-36.53)*340),-32768ll,32767ll)));r[0x3a]|=1;
    if((r[0x6a]&0x40)&&r[0x23]){std::vector<std::uint8_t> sample;auto add=[&](int at,int n){sample.insert(sample.end(),r.begin()+at,r.begin()+at+n);};if(r[0x23]&8)add(0x3b,6);if(r[0x23]&128)add(0x41,2);for(int i=0;i<3;++i)if(r[0x23]&(64>>i))add(0x43+i*2,2);for(auto value:sample){if(state.fifo.size()>=1024){state.fifo.pop_front();r[0x3a]|=0x10;}state.fifo.push_back(value);}}
    word(r,0x72,int(state.fifo.size()));
}
ModuleDevices::State& ModuleDevices::prepare(const CircuitPart& part,Microseconds now){
    const auto& d=ModuleRegistry::get(part.model);auto& s=states_[part.id];if(!s.initialized||s.model!=part.model)initialize(s,d);
    if(d.implementation=="registers")for(const auto& field:d.parameters)if(field.registerAddress>=0){const auto bits=std::uint64_t(std::llround(parameter(part,field.name,field.initial)*field.scale));for(int i=0;i<field.bytes;++i)s.reg[field.registerAddress+i]=std::uint8_t(bits>>((field.littleEndian?i:field.bytes-1-i)*8));}
    if(d.implementation=="bme280"){
        s.reg[0xf3]=now<s.resetUntil?1:0;if(s.pending&&now>=s.ready&&now>=s.resetUntil){sampleBme(s,part);s.pending=false;if((s.reg[0xf4]&3)!=3)s.reg[0xf4]&=0xfc;else{const Microseconds standby[]={500,62500,125000,250000,500000,1000000,10000,20000};s.nextSample=now+standby[(s.reg[0xf5]>>5)&7];}}
        if(!s.pending&&(s.reg[0xf4]&3)==3&&(!emulation_||now>=s.nextSample)&&now>=s.resetUntil){if(!emulation_)sampleBme(s,part);else{s.pending=true;s.ready=now+10000;}}
        if(s.pending)s.reg[0xf3]|=8;
    }else if(d.implementation=="mpu6050"&&now>=s.resetUntil&&!(s.reg[0x6b]&0x40)){
        if(now>=s.nextSample){sampleMpu(s,part);const unsigned rate=(s.reg[0x1a]&7)==0?8000:1000;s.nextSample=now+(1000000ull*(s.reg[0x19]+1)+rate-1)/rate;}
    }else if(d.implementation=="mfrc522"&&now>=s.resetUntil)s.reg[0x01]&=0xef;
    if(d.implementation=="vs1053b"&&s.pcm&&now>=s.pcmStart){auto seconds=(now-s.pcmStart)/1000000;s.reg[8]=seconds&255;s.reg[9]=seconds>>8;bool playing=now-s.pcmStart<Microseconds(s.pcm->size()/2)*1000000/s.pcmRate;s.reg[18]=playing&&!s.words[7]?0xe0:0;s.reg[19]=playing&&!s.words[7]?0xff:0;}
    if(d.implementation=="mpr121"){unsigned count=std::min(12,int(s.reg[0x5e]&15)),mask=unsigned(parameter(part,"touchMask",0))&((1u<<count)-1);if(mask!=s.words[0])s.words[1]=1;s.words[0]=mask;s.reg[0]=mask;s.reg[1]=mask>>8;for(int i=0;i<12;++i){int value=(mask&(1<<i))?800:1000;s.reg[4+i*2]=value;s.reg[5+i*2]=value>>8;s.reg[0x1e + i]=250;}}
    return s;
}
std::uint8_t ModuleDevices::read(State& s,const ModuleDefinition& d,std::uint8_t at){if(d.implementation=="mpr121"&&at==1)s.words[1]=0;
    if(d.implementation=="mpu6050"){if(at==0x74){if(s.fifo.empty())return 0;const auto value=s.fifo.front();s.fifo.pop_front();word(s.reg,0x72,int(s.fifo.size()));return value;}if(at==0x3a){const auto value=s.reg[at];s.reg[at]=0;return value;}}
    if(d.implementation=="mfrc522"){if(at==0x0a)return std::uint8_t(s.fifo.size());if(at==0x09){if(s.fifo.empty())return 0;const auto value=s.fifo.front();s.fifo.pop_front();return value;}}
    return s.reg[at];
}
void ModuleDevices::write(State& s,const CircuitPart& part,std::uint8_t at,std::uint8_t value,Microseconds now){
    const auto& d=ModuleRegistry::get(part.model);
    if(d.implementation=="mpr121"&&at==0x80&&value==0x63){initialize(s,d);return;}
    if(d.implementation=="registers"){if(std::find(d.readOnly.begin(),d.readOnly.end(),at)!=d.readOnly.end())return;for(const auto& field:d.parameters)if(field.registerAddress>=0&&at>=field.registerAddress&&at<field.registerAddress+field.bytes)return;}
    if(d.implementation=="bme280"){
        if(at==0xe0&&value==0xb6){initialize(s,d);s.resetUntil=now+(emulation_?2000:0);return;}
        if(at!=0xf2&&at!=0xf4&&at!=0xf5)return;s.reg[at]=value;
        if(at==0xf4&&(value&3)){auto over=[](unsigned bits){return bits?1u<<std::min(bits-1,4u):0;};const auto us=1250u+2300u*over((value>>5)&7)+2300u*over((value>>2)&7)+575u+2300u*over(s.reg[0xf2]&7)+575u;s.pending=true;s.ready=now+(emulation_?us:0);if(!emulation_){sampleBme(s,part);s.pending=false;if((value&3)!=3)s.reg[0xf4]&=0xfc;}}
        return;
    }
    if(d.implementation=="mpu6050"){
        if(at==0x75||(at>=0x3b&&at<=0x48)||at==0x72||at==0x73)return;
        if(at==0x6b&&(value&0x80)){initialize(s,d);s.resetUntil=now+(emulation_?100000:0);return;}
        if(at==0x74){if(s.fifo.size()<1024)s.fifo.push_back(value);word(s.reg,0x72,int(s.fifo.size()));return;}
        if(at==0x6a&&(value&4)){s.fifo.clear();word(s.reg,0x72,0);value&=0xfb;}
    }
    if(d.implementation=="mfrc522"){
        if(at==0x37)return;
        if(at==0x01&&value==0x0f){initialize(s,d);s.resetUntil=now+(emulation_?50000:0);s.reg[1]=now<s.resetUntil?0x10:0;return;}
        if(at==0x09){if(s.fifo.size()<64)s.fifo.push_back(value);else s.reg[6]|=0x10;return;}
        if(at==0x0a){if(value&0x80)s.fifo.clear();return;}
        if(at==0x04||at==0x05){s.reg[at]&=std::uint8_t(~value);return;}
        s.reg[at]=value;
        if(at==0x01&&value==0x03){const auto crc=crcA(std::vector<std::uint8_t>(s.fifo.begin(),s.fifo.end()));s.reg[0x22]=crc[0];s.reg[0x21]=crc[1];s.reg[5]|=4;}
        else if(at==0x0d&&(value&0x80)&&s.reg[1]==0x0c){radio(s,part);s.reg[0x0d]&=0x7f;}
        return;
    }
    s.reg[at]=value;
}
std::vector<std::uint8_t> ModuleDevices::i2c(const CircuitPart& part,const std::vector<std::uint8_t>& data,int count,Microseconds now){
    if(count>128||count< -1||data.size()>128)throw std::invalid_argument("Invalid module I2C buffer");auto& s=prepare(part,now);const auto& d=ModuleRegistry::get(part.model);std::vector<std::uint8_t> result;
    if(d.implementation=="wm8731"){if(count<0&&data.size()>=2){int reg=data[0]>>1;if(reg==15)initialize(s,d);else {s.reg[reg*2]=data[1];s.reg[reg*2+1]=data[0]&1;}}return result;}
    if(d.implementation=="pt2258"){
        if(count<0)for(auto byte:data){if(byte==0xc0){for(int i=0;i<7;++i)s.words[i]=0;continue;}if((byte&0xf8)==0xf8){s.words[6]=byte&1;continue;}int high=byte&0xf0,low=byte&15;const int tens[]={0x80,0x40,0x00,0x20,0x60,0xa0};for(int i=0;i<6;++i){if(high==tens[i]&&low<=7)s.words[i]=low*10+s.words[i]%10;if(high==tens[i]+0x10&&low<=9)s.words[i]=s.words[i]/10*10+low;}if(high==0xd0&&low<=7)for(int i=0;i<6;++i)s.words[i]=low*10+s.words[i]%10;if(high==0xe0&&low<=9)for(int i=0;i<6;++i)s.words[i]=s.words[i]/10*10+low;}return result;
    }
    if(d.implementation=="mcp4725"){
        if(count<0){if(data.size()>=2&&(data[0]&0xc0)==0){s.words[0]=((data[0]&15)<<8)|data[1];s.words[2]=(data[0]>>4)&3;}else if(data.size()>=3&&((data[0]&0xe0)==0x40||(data[0]&0xe0)==0x60)){s.words[0]=(data[1]<<4)|(data[2]>>4);s.words[2]=(data[0]>>1)&3;if((data[0]&0xe0)==0x60)s.words[1]=s.words[0];}}
        else {std::array<std::uint8_t,5> bytes{std::uint8_t(0x80|(s.words[2]<<1)),std::uint8_t(s.words[0]>>4),std::uint8_t(s.words[0]<<4),std::uint8_t(s.words[1]>>8),std::uint8_t(s.words[1])};for(int i=0;i<count;++i)result.push_back(bytes[i%5]);}return result;
    }
    if(count<0){if(!data.empty()){s.pointer=data.front();for(std::size_t i=1;i<data.size();++i){write(s,part,s.pointer,data[i],now);++s.pointer;}}}
    else for(int i=0;i<count;++i){const auto at=s.pointer;result.push_back(read(s,d,at));if(!(d.implementation=="mpu6050"&&at==0x74))++s.pointer;}
    return result;
}
void ModuleDevices::select(const CircuitPart& part,bool active){auto& s=states_[part.id];if(s.selected!=active){s.selected=active;s.command=false;s.stage=0;}}
void ModuleDevices::setAnalog(const std::string& id,const std::array<double,8>& volts,double reference){if(!std::isfinite(reference)||reference<=0||reference>5.5)throw std::invalid_argument("Invalid ADC reference");analog_[id]={volts,reference};}
std::uint8_t ModuleDevices::spi(const CircuitPart& part,std::uint8_t value,Microseconds now){
    auto& s=prepare(part,now);if(!s.selected)return 0xff;const auto& d=ModuleRegistry::get(part.model);
    if(d.implementation=="vs1053b"){
        if(s.dataMode){if(parameter(part,"midi",0)||s.words[7]){if(s.midiPadding){receiveMidi(part,{value},now);s.midiPadding=false;}else s.midiPadding=true;return 0xff;}if(s.codecBytes.size()>=16*1024*1024)throw std::runtime_error("VS1053 input buffer is full");s.codecBytes.push_back(value);return 0xff;}
        if(s.stage==0){s.reading=value==3;s.command=value==2||value==3;s.stage=s.command?1:0;return 0xff;}
        if(s.stage==1){s.pointer=value&15;s.stage=2;return 0xff;}
        if(s.reading){if(s.pointer==6){auto word=s.wram[s.wramAddress];auto result=std::uint8_t(s.stage==2?word>>8:word);if(s.stage==3)++s.wramAddress;s.stage=s.stage==2?3:0;return result;}auto result=s.reg[s.pointer*2+(s.stage==2?1:0)];s.stage=s.stage==2?3:0;return result;}
        if(s.stage==2){s.serialWord=std::uint16_t(value)<<8;s.stage=3;return 0xff;}s.serialWord|=value;s.stage=0;if(s.pointer==7)s.wramAddress=s.serialWord;if(s.pointer==6)s.wram[s.wramAddress++]=s.serialWord;if(s.pointer==10&&s.serialWord==0x50)s.midiPlugin=true;if(s.pointer==0&&(s.serialWord&8)){s.pcm.reset();s.codecBytes.clear();s.generation=++generationCounter_;s.serialWord&=~8;}if(s.pointer==0&&(s.serialWord&4)){initialize(s,d);return 0xff;}s.reg[s.pointer*2]=s.serialWord&255;s.reg[s.pointer*2+1]=s.serialWord>>8;return 0xff;
    }
    if(d.implementation=="ad9850"){
        // Serial data enters D7 least-significant bit first, W0 through W4.
        std::uint8_t byte=0;for(int bit=0;bit<8;++bit)byte|=((value>>bit)&1)<<(7-bit);
        if(s.stage<4){const unsigned shift=s.stage*8;s.words[2]=(s.words[2]&~(0xffu<<shift))|(std::uint32_t(byte)<<shift);}
        else s.words[3]=byte;
        s.stage=(s.stage+1)%5;return 0;
    }
    if(d.implementation=="mcp4822"||d.implementation=="mcp4922"||d.implementation=="ad9833"){
        s.serialWord=std::uint16_t((s.serialWord<<8)|value);if(++s.stage<2)return 0;s.stage=0;const auto word=s.serialWord;
        if(d.implementation=="ad9833"){
            if(!(word&0xc000))s.control=word;else if((word&0xc000)==0xc000)s.words[4+((word>>13)&1)]=word&0xfff;
            else {int reg=(word&0x8000)?1:0;if(s.control&0x2000){if(s.upperWord)s.words[reg]=(s.words[reg]&0x3fff)|((word&0x3fff)<<14);else s.words[reg]=(s.words[reg]&0x0fffc000)|(word&0x3fff);s.upperWord=!s.upperWord;}else if(s.control&0x1000)s.words[reg]=(s.words[reg]&0x3fff)|((word&0x3fff)<<14);else s.words[reg]=(s.words[reg]&0x0fffc000)|(word&0x3fff);}
        }else {int channel=(word>>15)&1;s.words[2+channel]=word;if(s.words[6]==0)s.words[channel]=word;}
        return 0;
    }
    if(d.implementation=="mcp3008"){
        if(s.stage==0){s.stage=value==1?1:0;return 0;}
        if(s.stage==1){const int channel=(value>>4)&7;const auto found=analog_.find(part.id);double voltage=0,reference=3.3;if(found!=analog_.end()){reference=found->second.reference;voltage=found->second.volts[channel];if(!(value&0x80))voltage-=found->second.volts[channel^1];}s.adc=std::isfinite(voltage)?int(std::clamp(voltage/reference*1024.,0.,1023.)):0;s.stage=2;return std::uint8_t(s.adc>>8);}
        s.stage=0;return std::uint8_t(s.adc);
    }
    if(!s.command){s.command=true;s.reading=value&0x80;s.pointer=d.implementation=="mfrc522"?((value&0x7e)>>1):d.implementation=="bme280"?(value|0x80):(value&0x7f);return 0;}
    const auto at=s.pointer;if(!(d.implementation=="mfrc522"&&at==0x09))++s.pointer;
    if(s.reading)return read(s,d,at);write(s,part,at,value,now);return 0;
}
void ModuleDevices::radio(State& s,const CircuitPart& part){
    const auto sent=std::vector<std::uint8_t>(s.fifo.begin(),s.fifo.end());s.fifo.clear();s.reg[6]=0;s.reg[0x0c]&=0xf8;const auto uid=cards_[part.id];std::vector<std::uint8_t> response;
    if((s.reg[0x14]&3)==3&&!uid.empty()&&!sent.empty()){
        if((s.reg[0x0d]&7)==7&&(sent[0]==0x52||(sent[0]==0x26&&!s.halted))){s.halted=false;response={std::uint8_t(uid.size()==7?0x44:4),0};}
        else if(sent[0]==0x50&&sent.size()==4){const auto crc=crcA({0x50,0});if(sent[1]==0&&sent[2]==crc[0]&&sent[3]==crc[1])s.halted=true;}
        else if(!s.halted&&sent.size()>=2&&(sent[0]==0x93||sent[0]==0x95)){
            if(sent[1]==0x20){if(sent[0]==0x93&&uid.size()==7)response={0x88,uid[0],uid[1],uid[2]};else if(sent[0]==0x95&&uid.size()==7)response={uid[3],uid[4],uid[5],uid[6]};else if(uid.size()>=4)response={uid[0],uid[1],uid[2],uid[3]};if(response.size()==4)response.push_back(response[0]^response[1]^response[2]^response[3]);}
            else if(sent[1]==0x70&&sent.size()==9){std::vector<std::uint8_t> block;if(uid.size()==7&&sent[0]==0x93)block={0x88,uid[0],uid[1],uid[2]};else if(uid.size()==7&&sent[0]==0x95)block={uid[3],uid[4],uid[5],uid[6]};else if(uid.size()==4)block=uid;if(block.size()==4){block.push_back(block[0]^block[1]^block[2]^block[3]);const auto crc=crcA(std::vector<std::uint8_t>(sent.begin(),sent.begin()+7));if(std::equal(block.begin(),block.end(),sent.begin()+2)&&sent[7]==crc[0]&&sent[8]==crc[1]){response={std::uint8_t(sent[0]==0x93&&uid.size()==7?4:0)};const auto answerCrc=crcA(response);response.push_back(answerCrc[0]);response.push_back(answerCrc[1]);}else s.reg[6]|=4;}}
        }
    }
    for(auto value:response)s.fifo.push_back(value);s.reg[4]|=response.empty()?1:0x30;
}
std::array<std::uint8_t,256> ModuleDevices::registers(const CircuitPart& part,Microseconds now){return prepare(part,now).reg;}
std::uint8_t ModuleDevices::crc8(const std::vector<std::uint8_t>& bytes){std::uint8_t crc=0;for(auto value:bytes){crc^=value;for(int bit=0;bit<8;++bit)crc=(crc>>1)^((crc&1)?0x8c:0);}return crc;}
}

std::optional<std::array<std::uint8_t,256>> thinkercad::ModuleDevices::peekRegisters(const std::string& id)const{
    const auto found=states_.find(id);if(found==states_.end()||!found->second.initialized)return std::nullopt;return found->second.reg;
}

std::vector<thinkercad::PeripheralSignal> thinkercad::ModuleDevices::outputSignals(const CircuitPart& part,const CircuitState& circuit,Microseconds now){
    const auto& d=ModuleRegistry::get(part.model);if(d.implementation!="mcp4725"&&d.implementation!="mcp4822"&&d.implementation!="mcp4922"&&d.implementation!="ad9833"&&d.implementation!="ad9850"&&d.implementation!="pt2258"&&d.implementation!="mpr121"&&d.implementation!="pcm5102a"&&d.implementation!="wm8731"&&d.implementation!="cs4272"&&d.implementation!="vs1053b")return {};
    auto& s=prepare(part,now);auto voltage=[&](const std::string& name,double fallback){auto it=circuit.voltage.find({part.id,name});return it!=circuit.voltage.end()&&it->second?*it->second:fallback;};double ground=voltage(d.ground,0),supply=voltage(d.power,ground)-ground;
    if(d.implementation=="vs1053b"){
        s.words[7]=parameter(part,"midi",0)||s.midiPlugin||voltage("GPIO1",ground)-ground>supply*.7;
        if(s.words[7]){
            std::erase_if(s.midiVoices,[&](const auto& entry){return entry.second.releasing&&now-entry.second.release>250000;});
            if(s.midiDirty||!s.pcm||now+10000>=s.pcmStart+Microseconds(s.pcm->size()/2)*1000000/s.pcmRate){
                auto pcm=std::make_shared<std::vector<float>>(5760,0);for(unsigned frame=0;frame<2880;++frame){double seconds=(now+frame*1000000./48000)*1e-6,left=0,right=0;for(const auto& [key,voice]:s.midiVoices){int note=key%128;double age=seconds-voice.start*1e-6;if(age<0)continue;double frequency=440*std::pow(2.,(note-69+s.midiBend[voice.channel])/12),phase=voice.phaseOffset+age*frequency;phase+=s.midiMod[voice.channel]/127.*.1*std::sin(age*31.4159265359);double wave=std::sin(phase*6.283185307179586),envelope=std::min(1.,age/.005);int family=voice.program/8;
                        if(family==0||family==3||family==4||family==13){wave=(wave+.3*std::sin(phase*12.566370614)+.1*std::sin(phase*18.84955592))/1.4;envelope*=std::exp(-age*(family==4?1:2.5));}
                        else if(family==1||family==14){wave=(wave+.4*std::sin(phase*6.283185307*2.76))/1.4;envelope*=std::exp(-age*4);}
                        else if(family==2||family==8){wave=0;for(int harmonic=1;harmonic<=15&&harmonic*frequency<20000;harmonic+=2)wave+=std::sin(phase*6.283185307*harmonic)/harmonic;wave*=.7;}
                        else if(family==5||family==6||family==7||family==10||family==11){wave=0;for(int harmonic=1;harmonic<=15&&harmonic*frequency<20000;++harmonic)wave+=std::sin(phase*6.283185307*harmonic)/harmonic;wave*=.5;if(family==11)envelope*=std::min(1.,age/.1);}
                        if(voice.channel==9||family==12||family==15){auto random=std::uint64_t(seconds*48000)*6364136223846793005ULL+std::uint64_t(note)*1442695040888963407ULL;wave=double((random>>32)&65535)/32767.5-1;if(voice.channel==9){envelope*=std::exp(-age*12);if(note==35||note==36)wave=std::sin(age*(60+180*std::exp(-age*30))*6.283185307);}}
                        if(voice.releasing){double release=seconds-voice.release*1e-6;envelope*=std::clamp(1-std::max(0.,release)/.25,0.,1.);}double level=voice.velocity/127.*s.midiVolume[voice.channel]/127.*envelope*.12;double pan=s.midiPan[voice.channel]/127.;left+=wave*level*std::sqrt(1-pan);right+=wave*level*std::sqrt(pan);
                    }(*pcm)[frame*2]=float(std::clamp(left,-.95,.95));(*pcm)[frame*2+1]=float(std::clamp(right,-.95,.95));}s.pcm=pcm;s.pcmStart=now;s.pcmRate=48000;s.midiDirty=false;
            }
        }
        std::vector<PeripheralSignal> out{{"DREQ",voltage("RESET",ground+supply)-ground<supply*.3?0:supply,0,0,0}};if(voltage("RESET",ground+supply)-ground<supply*.3)return out;if(s.pcm)for(unsigned channel=0;channel<2;++channel){PeripheralSignal value;value.pin=channel?"OUTR":"OUTL";value.voltage=supply/2;int attenuation=s.reg[23-channel];value.amplitude=std::pow(10.,-attenuation/40.)*std::min(1.5,supply/2);value.pcm=s.pcm;value.pcmStart=s.pcmStart;value.sampleRate=s.pcmRate;value.channel=channel;out.push_back(value);}return out;
    }
    if(d.implementation=="pcm5102a"||d.implementation=="wm8731"||d.implementation=="cs4272"){
        bool muted=d.implementation=="pcm5102a"?voltage("XSMT",ground)-ground<supply*.7:d.implementation=="wm8731"?!(s.reg[18]&1)||(s.reg[12]&0x88):s.reg[7]&1;if(muted||!s.pcm)return {};
        std::vector<PeripheralSignal> out;for(unsigned channel=0;channel<2;++channel){PeripheralSignal value;value.pin=channel?"OUTR":"OUTL";value.voltage=supply/2;value.amplitude=std::min(1.5,supply/2);value.pcm=s.pcm;value.pcmStart=s.pcmStart;value.sampleRate=s.pcmRate;value.channels=2;value.channel=channel;out.push_back(value);}return out;
    }
    if(d.implementation=="pt2258"){std::vector<PeripheralSignal> out;for(int i=0;i<6;++i){PeripheralSignal signal;signal.pin="OUT"+std::to_string(i+1);signal.input="IN"+std::to_string(i+1);signal.gain=s.words[6]?0:std::pow(10.,-double(s.words[i])/20);out.push_back(signal);}return out;}
    if(d.implementation=="mpr121")return s.words[1]?std::vector<PeripheralSignal>{{"IRQ",0,0,0,0}}:std::vector<PeripheralSignal>{};
    if(d.implementation=="mcp4725")return {{"VOUT",s.words[2]?0:supply*s.words[0]/4096.,0,0,0}};
    if(d.implementation=="ad9850"){
        if(voltage("RESET",ground)-ground>supply*.7){s.words.fill(0);s.stage=0;return {{"VOUT",.5,0,0,0}};}
        bool update=voltage("FQ_UD",ground)-ground>supply*.7;
        if(update&&!s.words[4]){s.words[0]=s.words[2];s.words[1]=s.words[3];}s.words[4]=update;
        if(s.words[1]&4)return {{"VOUT",.5,0,0,0}};
        PeripheralSignal output{"VOUT",.5,parameter(part,"mclk",125000000)*s.words[0]/4294967296.,.5,1};output.phase=(s.words[1]>>3)/32.;return {output};
    }
    if(d.implementation=="ad9833"){if(s.control&0x0180)return {{"VOUT",.3,0,0,0}};double frequency=parameter(part,"mclk",25000000)*s.words[(s.control>>11)&1]/268435456.;PeripheralSignal output{"VOUT",.3,frequency,.3,(s.control&0x20)?3:(s.control&2)?2:1};output.phase=s.words[4+((s.control>>10)&1)]/4096.;return {output};}
    s.words[6]=voltage("LDAC",ground)-ground>=supply*.5;if(!s.words[6]){s.words[0]=s.words[2];s.words[1]=s.words[3];}std::vector<PeripheralSignal> result;for(int i=0;i<2;++i){auto word=s.words[i];if(!(word&0x1000))continue;double reference=d.implementation=="mcp4822"?2.048:voltage(i?"VREFB":"VREFA",ground)-ground;double output=std::clamp((word&4095)*reference*((word&0x2000)?1:2)/4096.,0.,std::max(0.,supply-.05));result.push_back({i?"VOUTB":"VOUTA",output,0,0,0});}return result;
}

unsigned thinkercad::ModuleDevices::i2sWrite(const CircuitDocument& doc,const CircuitState& circuit,const std::string& board,int bclk,int lrck,int dout,unsigned rate,const std::vector<float>& pcm,Microseconds start){
    if(rate<8000||rate>384000||pcm.size()>65536||pcm.size()%2)throw std::invalid_argument("Invalid stereo I2S PCM frame");const auto profile=BoardProfile::get(doc.part(board).model);unsigned delivered=0;
    for(const auto& p:doc.parts())if(p.kind==PartKind::Module){const auto& d=ModuleRegistry::get(p.model);if(d.implementation!="pcm5102a"&&d.implementation!="wm8731"&&d.implementation!="cs4272")continue;
        auto v=circuit.voltage.find({p.id,d.power}),g=circuit.voltage.find({p.id,d.ground});if(v==circuit.voltage.end()||g==circuit.voltage.end()||!v->second||!g->second||*v->second-*g->second<d.minVoltage||*v->second-*g->second>d.maxVoltage)continue;
        if(!doc.connected({p.id,d.ground},{board,"GND"})||!doc.connected({p.id,"BCLK"},{board,profile.pinName(bclk)})||!doc.connected({p.id,"LRCK"},{board,profile.pinName(lrck)})||!doc.connected({p.id,"DIN"},{board,profile.pinName(dout)}))continue;
        auto& state=prepare(p,start);state.pcm=std::make_shared<const std::vector<float>>(pcm);state.pcmStart=start;state.pcmRate=rate;++delivered;
    }return delivered;
}

void thinkercad::ModuleDevices::captureI2sInputs(const CircuitDocument& doc,const CircuitState& circuit,Microseconds now){
    for(const auto& part:doc.parts())if(part.kind==PartKind::Module&&(part.model=="wm8731"||part.model=="cs4272")){auto& state=prepare(part,now);const auto& d=ModuleRegistry::get(part.model);auto voltage=[&](const std::string& pin){auto it=circuit.voltage.find({part.id,pin});return it!=circuit.voltage.end()&&it->second?*it->second:0.;};double supply=voltage(d.power)-voltage(d.ground);if(supply<d.minVoltage||supply>d.maxVoltage){state.adcPcm.clear();state.adcNext=now;continue;}
        std::array<float,2> sample{float(std::clamp((voltage("INL")-voltage(d.ground))/1.5,-1.,1.)),float(std::clamp((voltage("INR")-voltage(d.ground))/1.5,-1.,1.))};unsigned count=0;while(state.adcNext<=now&&count++<65536){state.adcPcm.push_back(sample);state.adcNext+=1000000./state.adcRate;}while(state.adcPcm.size()>16384)state.adcPcm.pop_front();
    }
}
std::vector<float> thinkercad::ModuleDevices::i2sRead(const CircuitDocument& doc,const CircuitState& circuit,const std::string& board,int bclk,int lrck,int din,unsigned rate,std::size_t frames){
    if(rate<8000||rate>384000||frames>16384)throw std::invalid_argument("Invalid I2S input frame");const auto profile=BoardProfile::get(doc.part(board).model);
    for(const auto& part:doc.parts())if(part.kind==PartKind::Module&&(part.model=="wm8731"||part.model=="cs4272")){
        if(!doc.connected({part.id,"DOUT"},{board,profile.pinName(din)})||!doc.connected({part.id,"BCLK"},{board,profile.pinName(bclk)})||!doc.connected({part.id,"LRCK"},{board,profile.pinName(lrck)})||!doc.connected({part.id,"GND"},{board,"GND"}))continue;auto& state=prepare(part,0);if(state.adcRate!=rate){state.adcRate=rate;state.adcPcm.clear();state.adcNext=0;}std::vector<float> result;for(std::size_t i=0;i<frames;++i){auto value=state.adcPcm.empty()?std::array<float,2>{}:state.adcPcm.front();if(!state.adcPcm.empty())state.adcPcm.pop_front();result.push_back(value[0]);result.push_back(value[1]);}return result;
    }return std::vector<float>(frames*2);
}

std::vector<thinkercad::ModuleDevices::CodecChunk> thinkercad::ModuleDevices::takeCodecChunks(){std::vector<CodecChunk> result;for(auto& [id,state]:states_)if(state.model=="vs1053b"&&!state.codecBytes.empty()){result.push_back({id,std::move(state.codecBytes),state.generation});state.codecBytes.clear();}return result;}
void thinkercad::ModuleDevices::setDecodedAudio(const CircuitPart& part,std::shared_ptr<const std::vector<float>> pcm,unsigned rate,Microseconds start,std::uint64_t generation){auto& state=prepare(part,start);if(state.generation!=generation)return;
    if(!pcm||rate<8000||rate>384000||pcm->size()%2)throw std::invalid_argument("Invalid decoded stereo PCM");
    if(state.pcm&&state.pcmRate==rate&&start>=state.pcmStart){
        auto consumed=std::min<std::size_t>(state.pcm->size()/2,std::size_t((start-state.pcmStart)*rate/1000000));
        if(consumed<state.pcm->size()/2){auto queued=std::make_shared<std::vector<float>>(state.pcm->begin()+consumed*2,state.pcm->end());queued->insert(queued->end(),pcm->begin(),pcm->end());pcm=queued;start=state.pcmStart+Microseconds(consumed)*1000000/rate;}
    }
    state.pcm=std::move(pcm);state.pcmRate=rate;state.pcmStart=start;state.reg[10]=(rate|1)&255;state.reg[11]=rate>>8;}

void thinkercad::ModuleDevices::receiveMidi(const CircuitPart& part,const std::vector<std::uint8_t>& bytes,Microseconds now){
    if(part.model!="vs1053b")return;auto& state=prepare(part,now);if(!parameter(part,"midi",0)&&!state.words[7])return;
    for(auto byte:bytes){if(byte>=0xf8){if(byte==0xff){state.midiVoices.clear();state.midiDirty=true;}continue;}if(byte&128){state.midiStatus=byte<0xf0?byte:-1;state.midiMessage.clear();continue;}if(state.midiStatus<0)continue;state.midiMessage.push_back(byte);int type=state.midiStatus&0xf0,channel=state.midiStatus&15,needed=(type==0xc0||type==0xd0)?1:2;if(int(state.midiMessage.size())<needed)continue;int first=state.midiMessage[0],second=needed==2?state.midiMessage[1]:0,key=channel*128+first;state.midiMessage.clear();
        if(type==0x90&&second){if(state.midiVoices.size()>=64)state.midiVoices.erase(state.midiVoices.begin());state.midiVoices[key]={unsigned(second),state.midiProgram[channel],unsigned(channel),now,0,false,false,0};}
        else if(type==0x80||(type==0x90&&!second)){auto note=state.midiVoices.find(key);if(note!=state.midiVoices.end()){if(state.midiSustain[channel])note->second.held=true;else {note->second.releasing=true;note->second.release=now;}}}
        else if(type==0xc0)state.midiProgram[channel]=first;
        else if(type==0xb0){if(first==7)state.midiVolume[channel]=second;if(first==10)state.midiPan[channel]=second;if(first==1)state.midiMod[channel]=second;if(first==64){state.midiSustain[channel]=second>=64;if(!state.midiSustain[channel])for(auto& [note,voice]:state.midiVoices)if(voice.channel==unsigned(channel)&&voice.held){voice.releasing=true;voice.release=now;voice.held=false;}}if(first==120||first==123)std::erase_if(state.midiVoices,[&](const auto& item){return item.second.channel==unsigned(channel);});}
        else if(type==0xe0){double bend=((first|(second<<7))-8192)/8192.*2,old=state.midiBend[channel];for(auto& [note,voice]:state.midiVoices)if(voice.channel==unsigned(channel)){double age=(now-voice.start)*1e-6;voice.phaseOffset+=age*440*(std::pow(2.,(note%128-69+old)/12)-std::pow(2.,(note%128-69+bend)/12));}state.midiBend[channel]=bend;}
        state.midiDirty=true;
    }
}

unsigned thinkercad::ModuleDevices::maximumSpiClock(const CircuitPart& part)const{const auto& model=ModuleRegistry::get(part.model);if(part.model!="vs1053b")return model.maxClock;auto it=states_.find(part.id);unsigned multiplier=0;bool data=false;if(it!=states_.end()){multiplier=(it->second.reg[7]>>5)&7;data=it->second.dataMode;}const double factors[]={1,2,2.5,3,3.5,4,4.5,5};return unsigned(12288000*factors[multiplier]/(data?4:7));}
