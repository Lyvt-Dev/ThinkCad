#include "thinkercad/moduledevices.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
namespace thinkercad {
void ModuleDevices::synchronizePower(const CircuitDocument& doc,const CircuitState& state){
    std::set<std::string> present;
    for(const auto& part:doc.parts())if(part.kind==PartKind::Module){present.insert(part.id);const auto& d=ModuleRegistry::get(part.model);auto voltage=[&](const std::string& pin)->std::optional<double>{auto it=state.voltage.find({part.id,pin});return it==state.voltage.end()?std::nullopt:it->second;};const auto v=voltage(d.power),g=voltage(d.ground);bool ready=v&&g&&*v-*g>=d.minVoltage&&*v-*g<=d.maxVoltage;if(ready&&d.implementation=="mfrc522"){const auto reset=voltage("RST");ready=reset&&*reset-*g>=.7*(*v-*g);}if(!ready)states_.erase(part.id);}
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
    const bool selected=state.selected;state=State{};state.initialized=true;state.model=d.id;state.selected=selected;
    for(const auto& [reg,value]:d.registers)state.reg[reg]=std::uint8_t(value);
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
    return s;
}
std::uint8_t ModuleDevices::read(State& s,const ModuleDefinition& d,std::uint8_t at){
    if(d.implementation=="mpu6050"){if(at==0x74){if(s.fifo.empty())return 0;const auto value=s.fifo.front();s.fifo.pop_front();word(s.reg,0x72,int(s.fifo.size()));return value;}if(at==0x3a){const auto value=s.reg[at];s.reg[at]=0;return value;}}
    if(d.implementation=="mfrc522"){if(at==0x0a)return std::uint8_t(s.fifo.size());if(at==0x09){if(s.fifo.empty())return 0;const auto value=s.fifo.front();s.fifo.pop_front();return value;}}
    return s.reg[at];
}
void ModuleDevices::write(State& s,const CircuitPart& part,std::uint8_t at,std::uint8_t value,Microseconds now){
    const auto& d=ModuleRegistry::get(part.model);
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
    if(count<0){if(!data.empty()){s.pointer=data.front();for(std::size_t i=1;i<data.size();++i){write(s,part,s.pointer,data[i],now);++s.pointer;}}}
    else for(int i=0;i<count;++i){const auto at=s.pointer;result.push_back(read(s,d,at));if(!(d.implementation=="mpu6050"&&at==0x74))++s.pointer;}
    return result;
}
void ModuleDevices::select(const CircuitPart& part,bool active){auto& s=states_[part.id];if(s.selected!=active){s.selected=active;s.command=false;s.stage=0;}}
void ModuleDevices::setAnalog(const std::string& id,const std::array<double,8>& volts,double reference){if(!std::isfinite(reference)||reference<=0||reference>5.5)throw std::invalid_argument("Invalid ADC reference");analog_[id]={volts,reference};}
std::uint8_t ModuleDevices::spi(const CircuitPart& part,std::uint8_t value,Microseconds now){
    auto& s=prepare(part,now);if(!s.selected)return 0xff;const auto& d=ModuleRegistry::get(part.model);
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
