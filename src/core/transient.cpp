#include "thinkercad/circuit.hpp"
#include "thinkercad/electronics.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace thinkercad {
void TransientSimulation::reset(){memory_={};contacts_.clear();now_=0;digitalSampling_=false;}
void TransientSimulation::setMaximumStep(Microseconds step){if(step<1||step>10000)throw std::invalid_argument("Time step outside 1..10000 us");maximumStep_=step;}
std::optional<CircuitDocument> TransientSimulation::contactDocument(const CircuitDocument& document,Microseconds at){
 std::optional<CircuitDocument> copy;
 for(const auto& p:document.parts())if(p.kind==PartKind::Button&&parameter(p,"bounceUs",0)>0){auto& c=contacts_[p.id];if(!c.initialized){c.initialized=true;c.requested=p.pressed;c.changed=at;}if(c.requested!=p.pressed){c.requested=p.pressed;c.changed=at;c.bouncing=true;}const auto elapsed=at-c.changed;const bool closed=c.bouncing&&elapsed<Microseconds(parameter(p,"bounceUs",0))?((elapsed/250)%2?c.requested:!c.requested):c.requested;if(closed!=p.pressed){if(!copy)copy=document;copy->part(p.id).pressed=closed;}}
 return copy;
}
CircuitState TransientSimulation::evaluate(const CircuitDocument& document,const Boards& boards){auto bounced=contactDocument(document,now_);const auto& effective=bounced?*bounced:document;auto state=effective.solveStep(boards,&memory_,1e-9,now_*1e-6);observeUltrasonic(document,state);for(int pass=0;pass<24;++pass){auto before=memory_.electronics;observeElectronics(document,state,memory_.electronics,now_);if(before==memory_.electronics)break;state=effective.solveStep(boards,&memory_,1e-9,now_*1e-6);}return state;}
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
    bool stepping=std::any_of(memory_.peripheralSignals.begin(),memory_.peripheralSignals.end(),[](const auto& entry){return !entry.second.empty();}),ultrasonic=false;for(const auto& p:document.parts()){ultrasonic|=p.kind==PartKind::Module&&(p.model=="hcsr04"||p.model=="ping3");stepping|=p.kind==PartKind::ServoMotor||p.kind==PartKind::DcMotor||p.kind==PartKind::Capacitor||p.kind==PartKind::Inductor||p.kind==PartKind::FunctionGenerator||p.kind==PartKind::Oscilloscope||p.kind==PartKind::LogicAnalyzer||p.kind==PartKind::CurrentProbe;}stepping|=ultrasonic;for(const auto& p:document.parts())stepping|=p.kind==PartKind::Button&&parameter(p,"bounceUs",0)>0;for(const auto& p:document.parts())stepping|=p.kind==PartKind::Module&&ModuleRegistry::get(p.model).bus=="electrical"&&
        p.model!="a4988"&&p.model!="uln2003"&&p.model!="stepper-4"&&p.model!="stepper-5"&&p.model!="stepper-6"&&p.model!="relay-5v"&&p.model!="limit-switch"&&p.model!="mh-z19b";
    for(const auto& [id,board]:boards)if(board)for(int pin:board->profile().gpio)stepping|=board->pin(pin).pwm||(digitalSampling_&&board->pin(pin).frequency);
    for(const auto& [id,signal]:memory_.infrared){auto edge=signal.nextEdge(now_);stepping|=edge>now_&&edge<=target;}
    if(!stepping){now_=target;auto result=evaluate(document,boards);if(sample)sample(now_,result);return result;}
    bool carrierSampling=digitalSampling_;for(const auto& p:document.parts())carrierSampling|=p.kind==PartKind::Oscilloscope||p.kind==PartKind::LogicAnalyzer||p.kind==PartKind::CurrentProbe;
    auto result=evaluate(document,boards);std::size_t steps=0;
    while(now_<target){
        if(++steps>10000)throw std::runtime_error("Transient step budget exceeded; reduce simulation speed or signal frequency.");
        auto step=std::min(maximumStep_,target-now_);
        for(const auto& p:document.parts())if(p.kind==PartKind::Module){if(p.model=="drum-pad"||p.model=="active-buzzer"||p.model=="as3340"||p.model=="cem3340"||p.model=="xr2206"||p.model=="icl8038"||p.model=="noise-generator"||p.model=="ay-3-8910"||p.model=="ym2149f"||p.model=="sn76489"||p.model=="pcm5102a"||p.model=="wm8731"||p.model=="cs4272"||p.model=="ssi2144"||p.model=="pt2399"||p.model=="fv1")step=std::min<Microseconds>(step,10);if(p.model=="encoder-600"&&std::abs(parameter(p,"rpm",0))>0)step=std::min<Microseconds>(step,std::max<Microseconds>(1,Microseconds(60000000/(std::abs(parameter(p,"rpm",0))*4800))));}
        for(const auto& [id,outputs]:memory_.peripheralSignals)for(const auto& output:outputs)if(output.frequency>0||output.pcm)step=std::min<Microseconds>(step,10);
        bool irEdge=false;for(const auto& [id,signal]:memory_.infrared){auto edge=signal.nextEdge(now_);if(edge>now_&&edge<=now_+step){step=edge-now_;irEdge=true;}}
        bool echoEdge=false;for(const auto& [id,sensor]:memory_.ultrasonic)for(const auto edge:{sensor.rise,sensor.fall})if(edge>now_)step=std::min(step,edge-now_);
        for(const auto& p:document.parts())if(p.kind==PartKind::DcMotor)step=std::min(step,std::max<Microseconds>(1,Microseconds(.25*parameter(p,"inertia",.00001)*p.value/.0004*1e6)));
        for(const auto& p:document.parts())if(p.kind==PartKind::FunctionGenerator)step=std::min(step,std::max<Microseconds>(1,Microseconds(1e6/(p.value*64))));
        for(const auto& [id,board]:boards)if(board)for(int gpio:board->profile().gpio){const auto& pin=board->pin(gpio);if(!pin.frequency||(pin.waveform&&(!carrierSampling||now_>=pin.waveformEnd)))continue;
            if(pin.waveform&&pin.waveformEdge){auto edge=pin.waveformEdge(now_);if(edge>now_)step=std::min(step,edge-now_);continue;}
            const double cycle=double(now_)*pin.frequency/1e6,phase=cycle-std::floor(cycle);
            const double edge=(std::floor(cycle)+(phase<pin.duty-1e-10?pin.duty:1))*1e6/pin.frequency;
            step=std::min(step,std::max<Microseconds>(1,Microseconds(std::ceil(edge-now_-1e-8))));
        }
        irEdge=false;for(const auto& [id,signal]:memory_.infrared)irEdge|=signal.nextEdge(now_)==now_+step;
        auto bounced=contactDocument(document,now_+step);
        result=(bounced?*bounced:document).solveStep(boards,&memory_,step*1e-6,(now_+step*0.5)*1e-6);
        if(!result.errors.empty())return result;
        for(const auto& p:document.parts()){
            if(p.kind==PartKind::DcMotor){auto i=result.branchCurrent.at(p.id);if(i){memory_.motorCurrent[p.id]=*i;const double inertia=parameter(p,"inertia",.00001),old=memory_.motorSpeed[p.id],load=parameter(p,"loadTorque",0);double drive=.02**i;double next=(old+step*1e-6*(drive-(old>0?load:old<0?-load:std::clamp(drive,-load,load)))/inertia)/(1+step*1e-6*.00001/inertia);if(old*next<0&&std::abs(drive)<=load)next=0;memory_.motorSpeed[p.id]=next;memory_.motorAngle[p.id]+=(old+next)*.5*step*1e-6;result.partValues[p.id]=next*60/(2*3.141592653589793);}}
            if(p.kind==PartKind::ServoMotor){
                const auto v=result.voltage.at({p.id,"VCC"}),g=result.voltage.at({p.id,"GND"}),signal=result.voltage.at({p.id,"SIGNAL"});
                const double t=(now_+step*.5)*1e-6;
                if(v&&g&&signal&&*v-*g>=4.5&&*v-*g<=6){
                    const bool level=*signal-*g>=2.;
                    if(level&&!memory_.servoLevel[p.id])memory_.servoRise[p.id]=t;
                    if(!level&&memory_.servoLevel[p.id]){
                        const double pulse=t-memory_.servoRise[p.id];
                        if(pulse>=.00045&&pulse<=.00255)memory_.servoPulse[p.id]=pulse;
                    }
                    memory_.servoLevel[p.id]=level;
                    // A hobby servo holds a physical shaft, not an instantaneous
                    // copy of the pulse command. Stop driving after missing pulses.
                    if(memory_.servoPulse.contains(p.id)&&t-memory_.servoRise[p.id]<.06){
                        auto position=memory_.servoAngle.try_emplace(p.id,90.).first;
                        auto& angle=position->second;
                        auto& velocity=memory_.servoVelocity[p.id];
                        const double targetAngle=std::clamp((memory_.servoPulse[p.id]*1e6-544)*180/1856,0.,180.);
                        const double dt=step*1e-6,error=targetAngle-angle;
                        const double acceleration=std::clamp(324*error-36*velocity,-4000.,4000.);
                        velocity=std::clamp(velocity+acceleration*dt,-500.,500.);
                        const double next=std::clamp(angle+velocity*dt,0.,180.);
                        if(error*(targetAngle-next)<=0){angle=targetAngle;velocity=0;}
                        else angle=next;
                    }else memory_.servoVelocity[p.id]=0;
                }else{memory_.servoLevel[p.id]=false;memory_.servoVelocity[p.id]=0;memory_.servoPulse.erase(p.id);}
                if(memory_.servoAngle.contains(p.id))result.partValues[p.id]=memory_.servoAngle[p.id];
            }
            if(p.kind==PartKind::Capacitor){const auto& a=result.voltage.at({p.id,"1"});const auto& b=result.voltage.at({p.id,"2"});if(a&&b)memory_.capacitorVoltage[p.id]=*a-*b;}
            if(p.kind==PartKind::Inductor){const auto& i=result.branchCurrent.at(p.id);if(i)memory_.inductorCurrent[p.id]=*i;}
        }
        now_+=step;if(irEdge){auto bouncedEnd=contactDocument(document,now_);result=(bouncedEnd?*bouncedEnd:document).solveStep(boards,&memory_,1e-9,now_*1e-6);}advanceAnalogDevices(document,result,memory_,now_);observeElectronics(document,result,memory_.electronics,now_);if(ultrasonic){for(const auto& [id,sensor]:memory_.ultrasonic)echoEdge|=sensor.rise==now_||sensor.fall==now_;if(echoEdge)result=document.solveStep(boards,&memory_,1e-9,now_*1e-6);observeUltrasonic(document,result);}if(sample)sample(now_,result);
    }
    return result;
}
}
