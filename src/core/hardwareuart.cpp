#include "thinkercad/uartbus.hpp"
#include "thinkercad/modules.hpp"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
namespace thinkercad {
namespace {
std::string nmea(std::string body){unsigned checksum=0;for(unsigned char ch:body)checksum^=ch;std::ostringstream out;out<<'$'<<body<<'*'<<std::uppercase<<std::hex<<std::setw(2)<<std::setfill('0')<<checksum<<"\r\n";return out.str();}
bool validNmea(const std::string& text){if(text.size()<7||text[0]!='$')return false;auto at=text.find('*');if(at==std::string::npos||at+3!=text.size())return false;unsigned checksum=0;for(std::size_t i=1;i<at;++i)checksum^=static_cast<unsigned char>(text[i]);auto digit=[](char c){if(c>='0'&&c<='9')return c-'0';if(c>='A'&&c<='F')return c-'A'+10;if(c>='a'&&c<='f')return c-'a'+10;return -1;};int a=digit(text[at+1]),b=digit(text[at+2]);return a>=0&&b>=0&&checksum==unsigned(a*16+b);}
std::string coordinate(double value,bool longitude){value=std::abs(value);int degrees=int(value);double minutes=std::round((value-degrees)*60*10000)/10000;if(minutes>=60){++degrees;minutes=0;}std::ostringstream out;out<<std::setfill('0')<<std::setw(longitude?3:2)<<degrees<<std::fixed<<std::setprecision(4)<<std::setw(7)<<minutes;return out.str();}
}
void UartBus::writeModules(const CircuitDocument& doc,const CircuitState& state,const std::string& board,int number,const std::vector<std::uint8_t>& bytes,Microseconds now,bool timed,const std::map<std::string,const Esp8266Board*>& boards){
    auto& source=require(board,number);const auto profile=BoardProfile::get(doc.part(board).model);
    std::erase_if(modules_,[&](const auto& entry){return std::none_of(doc.parts().begin(),doc.parts().end(),[&](const auto& p){return p.id==entry.first&&p.model==entry.second.model;});});
    auto voltage=[&](const std::string& id,const std::string& pin)->std::optional<double>{auto it=state.voltage.find({id,pin});return it==state.voltage.end()?std::nullopt:it->second;};
    for(const auto& part:doc.parts()){
        if(part.kind!=PartKind::Module)continue;
        const auto& d=ModuleRegistry::get(part.model);if(d.bus!="uart")continue;
        auto v=voltage(part.id,"VCC"),g=voltage(part.id,"GND");if(!v||!g||*v-*g<d.minVoltage||*v-*g>d.maxVoltage){modules_.erase(part.id);continue;}
        if(!doc.connected({board,"GND"},{part.id,"GND"}))continue;
        const unsigned baud=part.model=="sim7600"?115200:part.model=="fingerprint-uart"?57600:9600;
        const double logic=part.model=="sim800l"?2.8:3.3;
        bool path=doc.connected({board,profile.pinName(source.config.tx)},{part.id,"RX"});
        if(!path){auto at=boards.find(board);if(at!=boards.end()&&at->second){auto hi=*at->second,lo=hi;hi.pinMode(source.config.tx,PinMode::Output);lo.pinMode(source.config.tx,PinMode::Output);hi.digitalWrite(source.config.tx,true);lo.digitalWrite(source.config.tx,false);auto drivers=boards;drivers[board]=&hi;auto on=doc.solve(drivers);drivers[board]=&lo;auto off=doc.solve(drivers);auto endpoint=Endpoint{part.id,"RX"};auto a=on.voltage.find(endpoint),b=off.voltage.find(endpoint);path=on.errors.empty()&&off.errors.empty()&&a!=on.voltage.end()&&b!=off.voltage.end()&&a->second&&b->second&&*a->second-*g>=.7*logic&&*a->second-*g<=logic+.3&&*b->second-*g<.3*logic;}}
        if(!path)continue;
        auto input=voltage(part.id,"RX");if(!input||*input-*g<.7*logic||*input-*g>logic+.3||std::abs(double(source.config.baud)/baud-1)>.025||source.config.dataBits!=8||source.config.parity!=Parity::None||source.config.stopBits!=1)continue;
        auto& device=modules_[part.id];if(device.model!=part.model){device=ModulePort{};device.model=part.model;}
        auto reply=[&](const std::vector<std::uint8_t>& response){
            const auto start=timed?std::max(now,device.nextWrite):now;
            for(auto& [key,receiver]:ports_){if(receiver.config.rx<0)continue;const auto target=BoardProfile::get(doc.part(key.first).model);
                if(!doc.connected({part.id,"TX"},{key.first,target.pinName(receiver.config.rx)})||!doc.connected({part.id,"GND"},{key.first,"GND"}))continue;
                const bool valid=logic>=.7*target.voltage&&logic<=target.voltage+.3&&std::abs(double(receiver.config.baud)/baud-1)<=.025&&receiver.config.dataBits==8&&receiver.config.parity==Parity::None&&receiver.config.stopBits==1;
                Microseconds at=start;for(auto byte:response){auto ready=at+(timed?(10000000ull+baud-1)/baud:0);if(!valid)++receiver.errors.framing;else if(receiver.rx.size()>=256)++receiver.errors.overrun;else{Byte incoming{at,ready,byte,{part.id,0},true};const auto pos=std::upper_bound(receiver.rx.begin(),receiver.rx.end(),ready,[](auto t,const auto& pending){return t<pending.ready;});if(timed)for(auto& pending:receiver.rx)if(pending.source!=incoming.source&&pending.ready>at&&pending.start<ready){if(pending.valid)++receiver.errors.collision;pending.valid=false;incoming.valid=false;}if(!incoming.valid)++receiver.errors.collision;receiver.rx.insert(pos,incoming);}at=ready;}
            }
            device.nextWrite=start+response.size()*(timed?(10000000ull+baud-1)/baud:0);
        };
        auto text=[&](const std::string& response){reply({response.begin(),response.end()});};
        for(auto byte:bytes){
            if(device.input.size()>=512){device.input.clear();continue;}device.input.push_back(byte);
            if(part.model=="fingerprint-uart"){
                auto& in=device.input;if(in.size()==1&&in[0]!=0xef){in.clear();continue;}if(in.size()==2&&in[1]!=1){in.clear();continue;}if(in.size()<9)continue;
                const unsigned length=(in[7]<<8)|in[8];if(length<3||length>256){in.clear();continue;}if(in.size()<9+length)continue;
                unsigned sum=0;for(std::size_t i=6;i<in.size()-2;++i)sum+=in[i];bool valid=std::uint16_t(sum)==std::uint16_t((in[in.size()-2]<<8)|in.back())&&in[6]==1&&std::all_of(in.begin()+2,in.begin()+6,[](auto b){return b==255;});
                std::vector<std::uint8_t> payload{1};if(valid){const auto command=in[9];payload={0};bool present=parameter(part,"finger",0)!=0;
                    if(command==1)payload={std::uint8_t(present?0:2)};
                    else if(command==2)payload={std::uint8_t(present?0:6)};
                    else if(command==4){if(present&&parameter(part,"match",1)){unsigned id=unsigned(parameter(part,"template",1));payload={0,std::uint8_t(id>>8),std::uint8_t(id),0,100};}else payload={9};}
                    else if(command==0x1d)payload={0,0,1};
                    else if(command==0x13){if(length!=7||std::any_of(in.begin()+10,in.begin()+14,[](auto b){return b!=0;}))payload={0x13};}
                    else payload={0x1a};
                }
                std::vector<std::uint8_t> out{0xef,1,255,255,255,255,7,0,std::uint8_t(payload.size()+2)};out.insert(out.end(),payload.begin(),payload.end());sum=0;for(std::size_t i=6;i<out.size();++i)sum+=out[i];out.push_back(std::uint8_t(sum>>8));out.push_back(std::uint8_t(sum));reply(out);in.clear();continue;
            }
            if(byte!='\r'&&byte!='\n')continue;
            std::string command(device.input.begin(),device.input.end()-1);device.input.clear();if(command.empty())continue;
            if(part.model=="neo-6m"){
                if(!validNmea(command)||!command.starts_with("$PUBX,00*"))continue;
                auto latitude=parameter(part,"latitude",52.52),longitude=parameter(part,"longitude",13.405);bool fix=parameter(part,"fix",1)!=0;
                auto position=coordinate(latitude,false)+","+(latitude<0?"S":"N")+","+coordinate(longitude,true)+","+(longitude<0?"W":"E");
                unsigned seconds=unsigned(now/1000000%86400);std::ostringstream utc;utc<<std::setfill('0')<<std::setw(2)<<seconds/3600<<std::setw(2)<<seconds/60%60<<std::setw(2)<<seconds%60<<".00";
                std::ostringstream altitude;altitude<<std::fixed<<std::setprecision(1)<<parameter(part,"altitude",34);
                text(nmea("GPRMC,"+utc.str()+","+(fix?"A":"V")+","+position+",0.0,0.0,010100,,,"+(fix?"A":"N"))+nmea("GPGGA,"+utc.str()+","+position+","+(fix?"1,08":"0,00")+",1.0,"+altitude.str()+",M,0.0,M,,"));continue;
            }
            if(device.echo)text(command+"\r\n");
            std::string response="OK";
            if(command=="ATE0")device.echo=false;else if(command=="ATE1")device.echo=true;
            else if(command=="AT"){}else if(command=="ATI")response=(part.model=="sim800l"?"SIM800L":"SIM7600")+std::string(" ThinkCad offline test modem\r\nOK");
            else if(command=="AT+CPIN?")response="+CPIN: READY\r\nOK";
            else if(command=="AT+CSQ")response="+CSQ: "+std::to_string(int(parameter(part,"signal",20)))+",0\r\nOK";
            else if(command=="AT+CREG?")response=std::string("+CREG: 0,")+(parameter(part,"registered",1)?"1":"0")+"\r\nOK";
            else if(command=="AT+CMGF=1"){}else if(command=="AT+CMGL=\"ALL\"")response=parameter(part,"sms",0)?"+CMGL: 1,\"REC UNREAD\",\"TEST\",\"\",\"00/01/01,00:00:00+00\"\r\nThinkCad synthetic SMS\r\nOK":"OK";
            else response="ERROR";
            text("\r\n"+response+"\r\n");
        }
    }
}
}
