#include "thinkercad/circuit.hpp"
#include "thinkercad/electronics.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace thinkercad {
void TransientSimulation::reset(){memory_={};now_=0;digitalSampling_=false;}
void TransientSimulation::setMaximumStep(Microseconds step){if(step<1||step>10000)throw std::invalid_argument("Time step outside 1..10000 us");maximumStep_=step;}
CircuitState TransientSimulation::evaluate(const CircuitDocument& document,const Boards& boards){auto state=document.solveStep(boards,&memory_,1e-9,now_*1e-6);observeUltrasonic(document,state);for(int pass=0;pass<24;++pass){auto before=memory_.electronics;observeElectronics(document,state,memory_.electronics,now_);if(before==memory_.electronics)break;state=document.solveStep(boards,&memory_,1e-9,now_*1e-6);}return state;}
void TransientSimulation::observeUltrasonic(const CircuitDocument& document,const CircuitState& state){
    for(const auto& p:document.parts())if(p.kind==PartKind::Module&&(p.model=="hcsr04"||p.model=="ping3")){
        const auto v=state.voltage.at({p.id,"VCC"}),g=state.voltage.at({p.id,"GND"}),trigger=state.voltage.at({p.id,p.model=="ping3"?"SIG":"TRIG"});auto& sensor=memory_.ultrasonic[p.id];
        if(!v||!g||*v-*g<4.5||*v-*g>5.5||!trigger){sensor={};continue;}
        const double level=*trigger-*g;if(level>=2&&level<=5.5){if(!sensor.high){sensor.high=true;sensor.trigger=now_;}}
        else if(level>=-.3&&level<=.8&&sensor.high){sensor.high=false;if(now_-sensor.trigger>=(p.model=="ping3"?5u:10u)&&now_>=sensor.ready){sensor.rise=now_+200;sensor.fall=sensor.rise+Microseconds(std::llround(parameter(p,"distance",100)*2000000/34300));sensor.ready=now_+60000;}}
    }
}
std::pair<Microseconds,Microseconds> TransientSimulation::pulse(const CircuitDocument& doc,const CircuitState& state,const Boards& boards,const std::string& board,int pin,bool high,Microseconds timeout)const{
    const auto profile=BoardProfile::get(doc.part(board).model);const auto name=profile.pinName(pin);if(profile.pinNumber(name)<0||timeout>10000000)throw std::invalid_argument("Invalid pulse input or timeout");
    if(high)for(const auto& p:doc.parts())if(p.kind==PartKind::Module&&(p.model=="hcsr04"||p.model=="ping3")&&doc.connected({p.id,"GND"},{board,"GND"})){
        const auto v=state.voltage.at({p.id,"VCC"}),g=state.voltage.at({p.id,"GND"});if(!v||!g||*v-*g<4.5||*v-*g>5.5)continue;const auto it=memory_.ultrasonic.find(p.id);if(it!=memory_.ultrasonic.end()&&it->second.rise>now_&&it->second.fall-now_<=timeout){
            const auto on=doc.solveStep(boards,&memory_,1e-9,it->second.rise*1e-6);auto without=memory_;without.ultrasonic[p.id]={};const auto off=doc.solveStep(boards,&without,1e-9,it->second.rise*1e-6);
            if(on.level({board,name})==Level::High&&off.level({board,name})==Level::Low)return {it->second.fall-it->second.rise,it->second.fall-now_};
        }
    }
    return {0,timeout};
}
CircuitState TransientSimulation::advance(const CircuitDocument& document,const Boards& boards,Microseconds target,const std::function<void(Microseconds,const CircuitState&)>& sample){
    if(target<now_)throw std::invalid_argument("Transient clock cannot go backwards");
    bool stepping=!memory_.peripheralSignals.empty(),ultrasonic=false;for(const auto& p:document.parts()){ultrasonic|=p.kind==PartKind::Module&&(p.model=="hcsr04"||p.model=="ping3");stepping|=p.kind==PartKind::ServoMotor||p.kind==PartKind::DcMotor||p.kind==PartKind::Capacitor||p.kind==PartKind::Inductor||p.kind==PartKind::FunctionGenerator||p.kind==PartKind::Oscilloscope||p.kind==PartKind::LogicAnalyzer||p.kind==PartKind::CurrentProbe;}stepping|=ultrasonic;for(const auto& p:document.parts())stepping|=p.kind==PartKind::Module&&ModuleRegistry::get(p.model).bus=="electrical";
    for(const auto& [id,board]:boards)if(board)for(int pin:board->profile().gpio)stepping|=board->pin(pin).pwm||(digitalSampling_&&board->pin(pin).frequency);
    if(!stepping){now_=target;auto result=document.solve(boards);if(sample)sample(now_,result);return result;}
    auto result=evaluate(document,boards);std::size_t steps=0;
    while(now_<target){
        if(++steps>10000)throw std::runtime_error("Transient step budget exceeded; reduce simulation speed or signal frequency.");
        auto step=std::min(maximumStep_,target-now_);
        for(const auto& p:document.parts())if(p.kind==PartKind::Module){if(p.model=="drum-pad"||p.model=="active-buzzer"||p.model=="as3340"||p.model=="cem3340"||p.model=="xr2206"||p.model=="icl8038"||p.model=="noise-generator"||p.model=="ay-3-8910"||p.model=="ym2149f"||p.model=="sn76489"||p.model=="pcm5102a"||p.model=="wm8731"||p.model=="cs4272"||p.model=="ssi2144"||p.model=="pt2399"||p.model=="fv1")step=std::min<Microseconds>(step,10);if(p.model=="encoder-600"&&std::abs(parameter(p,"rpm",0))>0)step=std::min<Microseconds>(step,std::max<Microseconds>(1,Microseconds(60000000/(std::abs(parameter(p,"rpm",0))*4800))));}
        for(const auto& [id,outputs]:memory_.peripheralSignals)for(const auto& output:outputs)if(output.frequency>0||output.pcm)step=std::min<Microseconds>(step,10);
        bool echoEdge=false;for(const auto& [id,sensor]:memory_.ultrasonic)for(const auto edge:{sensor.rise,sensor.fall})if(edge>now_)step=std::min(step,edge-now_);
        for(const auto& p:document.parts())if(p.kind==PartKind::DcMotor)step=std::min(step,std::max<Microseconds>(1,Microseconds(.25*parameter(p,"inertia",.00001)*p.value/.0004*1e6)));
        for(const auto& p:document.parts())if(p.kind==PartKind::FunctionGenerator)step=std::min(step,std::max<Microseconds>(1,Microseconds(1e6/(p.value*64))));
        for(const auto& [id,board]:boards)if(board)for(int gpio:board->profile().gpio){const auto& pin=board->pin(gpio);if(!pin.frequency)continue;
            const double cycle=double(now_)*pin.frequency/1e6,phase=cycle-std::floor(cycle);
            const double edge=(std::floor(cycle)+(phase<pin.duty-1e-10?pin.duty:1))*1e6/pin.frequency;
            step=std::min(step,std::max<Microseconds>(1,Microseconds(std::ceil(edge-now_-1e-8))));
        }
        result=document.solveStep(boards,&memory_,step*1e-6,(now_+step*0.5)*1e-6);
        if(!result.errors.empty())return result;
        for(const auto& p:document.parts()){
            if(p.kind==PartKind::DcMotor){auto i=result.branchCurrent.at(p.id);if(i){memory_.motorCurrent[p.id]=*i;const double inertia=parameter(p,"inertia",.00001),old=memory_.motorSpeed[p.id],load=parameter(p,"loadTorque",0);double drive=.02**i;double next=(old+step*1e-6*(drive-(old>0?load:old<0?-load:std::clamp(drive,-load,load)))/inertia)/(1+step*1e-6*.00001/inertia);if(old*next<0&&std::abs(drive)<=load)next=0;memory_.motorSpeed[p.id]=next;memory_.motorAngle[p.id]+=(old+next)*.5*step*1e-6;result.partValues[p.id]=next*60/(2*3.141592653589793);}}
            if(p.kind==PartKind::ServoMotor){auto v=result.voltage.at({p.id,"VCC"}),g=result.voltage.at({p.id,"GND"}),signal=result.voltage.at({p.id,"SIGNAL"});if(v&&g&&signal&&*v-*g>=4.5&&*v-*g<=6){bool level=*signal-*g>=2.;const double t=(now_+step*.5)*1e-6;if(level&&!memory_.servoLevel[p.id])memory_.servoRise[p.id]=t;if(!level&&memory_.servoLevel[p.id]){double pulse=t-memory_.servoRise[p.id];if(pulse>=.00045&&pulse<=.00255)memory_.servoPulse[p.id]=pulse;}memory_.servoLevel[p.id]=level;if(memory_.servoPulse.contains(p.id))result.partValues[p.id]=std::clamp((memory_.servoPulse[p.id]*1e6-544)*180/1856,0.,180.);}}
            if(p.kind==PartKind::Capacitor){const auto& a=result.voltage.at({p.id,"1"});const auto& b=result.voltage.at({p.id,"2"});if(a&&b)memory_.capacitorVoltage[p.id]=*a-*b;}
            if(p.kind==PartKind::Inductor){const auto& i=result.branchCurrent.at(p.id);if(i)memory_.inductorCurrent[p.id]=*i;}
        }
        now_+=step;advanceAnalogDevices(document,result,memory_,now_);observeElectronics(document,result,memory_.electronics,now_);if(ultrasonic){for(const auto& [id,sensor]:memory_.ultrasonic)echoEdge|=sensor.rise==now_||sensor.fall==now_;if(echoEdge)result=document.solveStep(boards,&memory_,1e-9,now_*1e-6);observeUltrasonic(document,result);}if(sample)sample(now_,result);
    }
    return result;
}
}
