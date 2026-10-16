#include "thinkercad/circuit.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace thinkercad;
int checks=0;
void check(bool ok,const char* msg){++checks;if(!ok)throw std::runtime_error(msg);}
void healthy(const CircuitState& s){if(!s.errors.empty())throw std::runtime_error(s.errors.front());}
double voltage(const CircuitState& s,const std::string& p,const std::string& pin){healthy(s);return s.voltage.at({p,pin}).value();}
struct Bench{CircuitDocument d;TransientSimulation sim;std::string ground,source;Bench(){d.removePart("board");ground=d.addPart(PartKind::Ground,0,0);source=d.addPart(PartKind::PowerSupply,0,0);d.part(source).value=5;d.connect({source,"-"},{ground,"GND"});}void g(std::string p,std::string pin){d.connect({p,pin},{ground,"GND"});}void v(std::string p,std::string pin){d.connect({p,pin},{source,"+"});}};
int main(){try{
    for(const auto& model:{"arduino-uno","arduino-nano","esp8266","esp32","esp32-s3"}){
        const auto profile=BoardProfile::get(model);Esp8266Board board;board.configure(model);
        check(!profile.supportsPwm(-1)&&!profile.supportsPwm(999),"unknown and power pins have no PWM marker");
        for(int pin:profile.gpio){
            const bool inputOnly=std::find(profile.inputOnly.begin(),profile.inputOnly.end(),pin)!=profile.inputOnly.end();
            if(inputOnly){check(!profile.supportsPwm(pin),"input-only GPIO has no PWM marker");continue;}
            board.analogWrite(pin,128);
            check(profile.supportsPwm(pin)==board.pin(pin).pwm,"PWM capability matches simulated output on every board pin");
        }
        if(profile.arduino())for(int pin=0;pin<22;++pin)
            check(profile.supportsPwm(pin)==(pin==3||pin==5||pin==6||pin==9||pin==10||pin==11),"classic AVR PWM pin map");
    }
    {Bench b;auto r=b.d.addPart(PartKind::Resistor,0,0),c=b.d.addPart(PartKind::Capacitor,0,0);b.d.part(r).value=1000;b.d.part(c).value=10;b.v(r,"1");b.d.connect({r,"2"},{c,"1"});b.g(c,"2");auto s=b.sim.advance(b.d,{},10000);check(std::abs(voltage(s,c,"1")-5*(1-std::exp(-1)))<0.015,"RC time constant");check(std::abs(*s.branchCurrent.at(c)-*s.branchCurrent.at(r))<1e-8,"RC KCL");b.d.part(b.source).value=0;s=b.sim.advance(b.d,{},20000);check(std::abs(voltage(s,c,"1")-5*(1-std::exp(-1))*std::exp(-1))<0.015,"RC discharge with retained state");}
    {Bench b;auto r=b.d.addPart(PartKind::Resistor,0,0),l=b.d.addPart(PartKind::Inductor,0,0);b.d.part(r).value=10;b.d.part(l).value=100;b.v(r,"1");b.d.connect({r,"2"},{l,"1"});b.g(l,"2");auto s=b.sim.advance(b.d,{},10000);check(std::abs(*s.branchCurrent.at(l)-5/10.1*(1-std::exp(-1.01)))<.002,"RL time constant");}
    {Bench b;auto r=b.d.addPart(PartKind::Resistor,0,0),c=b.d.addPart(PartKind::Capacitor,0,0);b.v(c,"1");b.g(c,"2");b.v(r,"1");b.g(r,"2");b.d.part(r).value=1000;auto s=b.sim.advance(b.d,{},100);check(std::abs(*s.branchCurrent.at(c)-.5)<1e-8,"parallel capacitor current uses its own companion, not resistor");}
    for(auto kind:{PartKind::NpnTransistor,PartKind::PnpTransistor,PartKind::NMosfet,PartKind::PMosfet}){Bench b;bool p=kind==PartKind::PnpTransistor||kind==PartKind::PMosfet,mos=kind==PartKind::NMosfet||kind==PartKind::PMosfet;auto t=b.d.addPart(kind,0,0),load=b.d.addPart(PartKind::Resistor,0,0),drive=b.d.addPart(PartKind::Resistor,0,0);b.d.part(load).value=1000;b.d.part(drive).value=10000;std::string out=mos?"D":"C",input=mos?"G":"B",common=mos?"S":"E";if(p){b.v(t,common);b.g(load,"2");}else{b.g(t,common);b.v(load,"2");}auto control=b.d.addPart(PartKind::PowerSupply,0,0);b.g(control,"-");b.d.part(control).value=p?0:5;b.d.connect({control,"+"},{drive,"1"});b.d.connect({load,"1"},{t,out});b.d.connect({drive,"2"},{t,input});auto s=b.d.solve(TransientSimulation::Boards{});double on=voltage(s,t,out);check(p?on>4.8:on<.2,"transistor switch on");b.d.part(control).value=p?5:0;s=b.d.solve(TransientSimulation::Boards{});double off=voltage(s,t,out);check(p?off<.001:off>4.999,"transistor switch off");
    }
    {Bench b;auto op=b.d.addPart(PartKind::OpAmp,0,0),in=b.d.addPart(PartKind::AnalogSource,0,0);b.d.part(in).value=2;b.v(op,"V+");b.g(op,"V-");b.g(in,"GND");b.d.connect({in,"OUT"},{op,"IN+"});b.d.connect({op,"OUT"},{op,"IN-"});auto s=b.d.solve(TransientSimulation::Boards{});check(std::abs(voltage(s,op,"OUT")-2)<.001,"op-amp voltage follower");}
    {CircuitDocument d;d.part("board").model="arduino-uno";Esp8266Board board;board.configure("arduino-uno");board.analogWrite(5,128);auto c=d.addPart(PartKind::Capacitor,0,0),r=d.addPart(PartKind::Resistor,0,0);d.part(r).value=1000;d.part(c).value=10;d.connect({"board","D5"},{r,"1"});d.connect({r,"2"},{c,"1"});d.connect({c,"2"},{"board","GND"});TransientSimulation sim;double lo=5,hi=0;auto s=sim.advance(d,{{"board",&board}},100000,[&](auto t,const auto& sample){if(t>90000){auto v=voltage(sample,c,"1");lo=std::min(lo,v);hi=std::max(hi,v);}});check(voltage(s,c,"1")>2.4&&voltage(s,c,"1")<2.6&&hi-lo<.15,"PWM RC low-pass average and ripple");check(board.pin(5).frequency==980,"Arduino D5 frequency");board.analogWrite(4,64);check(!board.pin(4).pwm&&!board.pin(4).output,"non-PWM AVR pin fallback");}
    {Bench b;auto g=b.d.addPart(PartKind::FunctionGenerator,0,0);b.g(g,"GND");b.d.part(g).value=100;b.d.part(g).parameters={{"offset",1},{"amplitude",2}};auto s=b.sim.advance(b.d,{},2500);check(std::abs(voltage(s,g,"OUT")-3)<.01,"function generator sine peak");}
    {Bench b;auto probe=b.d.addPart(PartKind::CurrentProbe,0,0),r=b.d.addPart(PartKind::Resistor,0,0);b.d.part(r).value=1000;b.v(probe,"+");b.d.connect({probe,"-"},{r,"1"});b.g(r,"2");auto s=b.d.solve(TransientSimulation::Boards{});healthy(s);check(std::abs(*s.branchCurrent.at(probe)-5/1000.001)<1e-10,"series current probe burden and polarity");b.d.connect({probe,"-"},{b.ground,"GND"});check(!b.d.solve(TransientSimulation::Boards{}).errors.empty(),"probe across supply reports range error");}
    {Bench b;auto cap=b.d.addPart(PartKind::Capacitor,0,0);auto s=b.sim.advance(b.d,{},100);check(!s.voltage.at({cap,"1"})&&!s.branchCurrent.at(cap),"isolated capacitor remains floating");}
    {Bench b;auto r=b.d.addPart(PartKind::Resistor,0,0),l=b.d.addPart(PartKind::Inductor,0,0),c=b.d.addPart(PartKind::Capacitor,0,0);b.d.part(r).value=5;b.d.part(l).value=100;b.d.part(c).value=10;b.v(r,"1");b.d.connect({r,"2"},{l,"1"});b.d.connect({l,"2"},{c,"1"});b.g(c,"2");b.sim.setMaximumStep(10);auto s=b.sim.advance(b.d,{},3142);check(std::abs(voltage(s,c,"1")-9.622)<.12,"underdamped RLC first peak");}
    {Bench b;auto op=b.d.addPart(PartKind::OpAmp,0,0),in=b.d.addPart(PartKind::AnalogSource,0,0),feedback=b.d.addPart(PartKind::Resistor,0,0),r=b.d.addPart(PartKind::Resistor,0,0);b.d.part(in).value=1;b.d.part(feedback).value=b.d.part(r).value=10000;b.v(op,"V+");b.g(op,"V-");b.g(in,"GND");b.d.connect({in,"OUT"},{op,"IN+"});b.d.connect({op,"OUT"},{feedback,"1"});b.d.connect({feedback,"2"},{op,"IN-"});b.d.connect({r,"1"},{op,"IN-"});b.g(r,"2");auto s=b.d.solve(TransientSimulation::Boards{});check(std::abs(voltage(s,op,"OUT")-2)<.001,"op-amp closed-loop gain of two");b.d.part(in).value=3.3;s=b.d.solve(TransientSimulation::Boards{});check(voltage(s,op,"OUT")>4.93&&voltage(s,op,"OUT")<4.951,"op-amp rail saturation under load");}
    {Bench b;auto op=b.d.addPart(PartKind::OpAmp,0,0);b.g(op,"V-");auto s=b.d.solve(TransientSimulation::Boards{});check(!s.voltage.at({op,"OUT"}),"unpowered op-amp output is floating");}
    std::cout<<checks<<" transient electrical checks passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
