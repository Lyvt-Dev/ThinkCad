#include "thinkercad/circuit.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace thinkercad {
void TransientSimulation::reset(){memory_={};now_=0;}
void TransientSimulation::setMaximumStep(Microseconds step){if(step<1||step>10000)throw std::invalid_argument("Time step outside 1..10000 us");maximumStep_=step;}
CircuitState TransientSimulation::evaluate(const CircuitDocument& document,const Boards& boards)const{return document.solveStep(boards,&memory_,1e-9,now_*1e-6);}
CircuitState TransientSimulation::advance(const CircuitDocument& document,const Boards& boards,Microseconds target,const std::function<void(Microseconds,const CircuitState&)>& sample){
    if(target<now_)throw std::invalid_argument("Transient clock cannot go backwards");
    bool stepping=false;for(const auto& p:document.parts())stepping|=p.kind==PartKind::ServoMotor||p.kind==PartKind::DcMotor||p.kind==PartKind::Capacitor||p.kind==PartKind::Inductor||p.kind==PartKind::FunctionGenerator||p.kind==PartKind::Oscilloscope||p.kind==PartKind::LogicAnalyzer||p.kind==PartKind::CurrentProbe;
    for(const auto& [id,board]:boards)if(board)for(int pin:board->profile().gpio)stepping|=board->pin(pin).pwm;
    if(!stepping){now_=target;auto result=document.solve(boards);if(sample)sample(now_,result);return result;}
    auto result=evaluate(document,boards);std::size_t steps=0;
    while(now_<target){
        if(++steps>10000)throw std::runtime_error("Transient step budget exceeded; reduce simulation speed or signal frequency.");
        auto step=std::min(maximumStep_,target-now_);
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
            if(p.kind==PartKind::DcMotor){auto i=result.branchCurrent.at(p.id);if(i){memory_.motorCurrent[p.id]=*i;const double inertia=parameter(p,"inertia",.00001),old=memory_.motorSpeed[p.id],load=parameter(p,"loadTorque",0);double drive=.02**i;double next=(old+step*1e-6*(drive-(old>0?load:old<0?-load:std::clamp(drive,-load,load)))/inertia)/(1+step*1e-6*.00001/inertia);if(old*next<0&&std::abs(drive)<=load)next=0;memory_.motorSpeed[p.id]=next;result.partValues[p.id]=next*60/(2*3.141592653589793);}}
            if(p.kind==PartKind::ServoMotor){auto v=result.voltage.at({p.id,"VCC"}),g=result.voltage.at({p.id,"GND"}),signal=result.voltage.at({p.id,"SIGNAL"});if(v&&g&&signal&&*v-*g>=4.5&&*v-*g<=6){bool level=*signal-*g>=2.;const double t=(now_+step*.5)*1e-6;if(level&&!memory_.servoLevel[p.id])memory_.servoRise[p.id]=t;if(!level&&memory_.servoLevel[p.id]){double pulse=t-memory_.servoRise[p.id];if(pulse>=.00045&&pulse<=.00255)memory_.servoPulse[p.id]=pulse;}memory_.servoLevel[p.id]=level;if(memory_.servoPulse.contains(p.id))result.partValues[p.id]=std::clamp((memory_.servoPulse[p.id]*1e6-544)*180/1856,0.,180.);}}
            if(p.kind==PartKind::Capacitor){const auto& a=result.voltage.at({p.id,"1"});const auto& b=result.voltage.at({p.id,"2"});if(a&&b)memory_.capacitorVoltage[p.id]=*a-*b;}
            if(p.kind==PartKind::Inductor){const auto& i=result.branchCurrent.at(p.id);if(i)memory_.inductorCurrent[p.id]=*i;}
        }
        now_+=step;if(sample)sample(now_,result);
    }
    return result;
}
}
