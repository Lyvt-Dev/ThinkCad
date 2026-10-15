#include "thinkercad/circuit.hpp"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>
#include <tuple>
namespace thinkercad {
const std::vector<ComponentSpec>& componentCatalog(){static const std::vector<ComponentSpec> types{
{PartKind::Esp8266,"Mikrocontroller","",0,0,1e9,false},{PartKind::Oled,"OLED SSD1306","",60,60,61,false},{PartKind::Led,"LED","",220,0,1e9,false},{PartKind::Resistor,"Widerstand"," Ω",220,1,1e9,true},{PartKind::Button,"Taster","",220,0,1e9,false},{PartKind::AnalogSource,"Analogquelle"," V",0.5,0,3.3,true},
{PartKind::Potentiometer,"Potentiometer 10 kΩ"," %",50,0,100,true},{PartKind::Photoresistor,"Fotowiderstand"," lx",100,0,100000,true},{PartKind::Thermistor,"NTC 10k B3950"," °C",25,-40,125,true},{PartKind::SlideSwitch,"Schiebeschalter SPDT","",0,0,1,false},{PartKind::TiltSwitch,"Neigungsschalter","",0,0,1,false},{PartKind::ReedSwitch,"Reedkontakt","",0,0,1,false},{PartKind::Diode,"Siliziumdiode","",0,0,1,false},{PartKind::Battery3V,"3-V-Batterie (ideal)","",3,3,3,false},{PartKind::Battery9V,"9-V-Batterie (ideal)","",9,9,9,false},{PartKind::PowerSupply,"Gleichspannungsquelle"," V",5,0,12,true},{PartKind::RGBLed,"RGB-LED (Kathode)","",0,0,1,false},{PartKind::SevenSegment,"7-Segment (Kathode)","",0,0,1,false},{PartKind::Voltmeter,"Voltmeter (ideal)","",0,0,1,false},{PartKind::Breadboard30,"Breadboard 300 Kontakte","",0,0,1,false}
};return types;}
const ComponentSpec& componentSpec(PartKind kind){for(const auto& type:componentCatalog())if(type.kind==kind)return type;throw std::invalid_argument("Unknown component type");}
namespace {
struct Nodes {
    std::map<Endpoint,int> index;
    std::vector<int> parent;
    int find(int a) { return parent[a]==a?a:parent[a]=find(parent[a]); }
    void join(const Endpoint& a,const Endpoint& b) { int x=find(index.at(a)),y=find(index.at(b));if(x!=y)parent[y]=x; }
    explicit Nodes(const CircuitDocument& doc) {
        for(const auto& p:doc.parts())for(const auto& pin:CircuitDocument::pins(p))index.emplace(Endpoint{p.id,pin},static_cast<int>(index.size()));
        parent.resize(index.size());std::iota(parent.begin(),parent.end(),0);
        for(const auto& part:doc.parts())if(part.kind==PartKind::Esp8266){
            const auto profile=BoardProfile::get(part.model);
            if(!profile.arduino()&&profile.id!="esp8266")join({part.id,"A0"},{part.id,profile.pinName(profile.defaultAdc)});
            join({part.id,"GND"},{part.id,"GND2"});
            if(!profile.arduino())join({part.id,"3V3"},{part.id,"3V3_2"});
        }
        for(const auto& w:doc.wires())join(w.a,w.b);
        for(const auto& p:doc.parts()){
            if((p.kind==PartKind::Button||p.kind==PartKind::TiltSwitch||p.kind==PartKind::ReedSwitch)&&p.pressed)join({p.id,"1"},{p.id,"2"});
            if(p.kind==PartKind::SlideSwitch)join({p.id,"C"},{p.id,p.pressed?"2":"1"});
            if(p.kind==PartKind::Breadboard30)for(int n=1;n<=30;++n)for(char start:{'a','f'})for(int j=1;j<5;++j)join({p.id,std::string(1,start)+std::to_string(n)},{p.id,std::string(1,start+j)+std::to_string(n)});
        }
    }
    int node(const Endpoint& e) {return find(index.at(e));}
};
struct Branch {int a,b;double resistance,offset=0;};
std::vector<double> linearSolve(std::vector<std::vector<double>> a,std::vector<double> b) {
    const auto n=b.size();
    for(std::size_t k=0;k<n;++k) {
        auto pivot=k;for(auto i=k+1;i<n;++i)if(std::abs(a[i][k])>std::abs(a[pivot][k]))pivot=i;
        if(std::abs(a[pivot][k])<1e-15)throw std::runtime_error("Singular resistor network");
        std::swap(a[k],a[pivot]);std::swap(b[k],b[pivot]);
        for(auto i=k+1;i<n;++i){const auto f=a[i][k]/a[k][k];for(auto j=k;j<n;++j)a[i][j]-=f*a[k][j];b[i]-=f*b[k];}
    }
    std::vector<double> x(n);
    for(std::size_t i=n;i-->0;){double v=b[i];for(auto j=i+1;j<n;++j)v-=a[i][j]*x[j];x[i]=v/a[i][i];}
    return x;
}
}
Level CircuitState::level(const Endpoint& e) const {
    const auto found=voltage.find(e);if(found==voltage.end()||!found->second)return Level::Floating;
    const auto v=*found->second;const double supply=logicVoltage.contains(e)?logicVoltage.at(e):3.3;if(v<=supply*(supply>=4.5?0.3:0.25))return Level::Low;if(v>=supply*(supply>=4.5?0.6:0.75))return Level::High;return Level::Floating;
}
CircuitDocument::CircuitDocument() {parts_.push_back({"board",PartKind::Esp8266,80,80,0,false,"#fb586b","esp8266"});}
std::string CircuitDocument::boardId()const{
    for(const auto& p:parts_)if(p.kind==PartKind::Esp8266&&p.id==activeBoard_)return p.id;
    for(const auto& p:parts_)if(p.kind==PartKind::Esp8266)return p.id;
    return {};
}
void CircuitDocument::setActiveBoard(const std::string& id){
    if(part(id).kind!=PartKind::Esp8266)throw std::invalid_argument("Not a microcontroller");
    activeBoard_=id;
}
std::vector<std::string> CircuitDocument::pins(PartKind k) {
    if(k==PartKind::Esp8266)return {"3V3","GND","GPIO16","GPIO5","GPIO4","GPIO0","GPIO2","A0","GND2","GPIO14","GPIO12","GPIO13","GPIO15","GPIO3","GPIO1","3V3_2"};
    if(k==PartKind::Oled)return {"GND","VCC","SCL","SDA"};
    if(k==PartKind::Led||k==PartKind::Diode)return {"A","K"};
    if(k==PartKind::Potentiometer)return {"1","W","2"};
    if(k==PartKind::SlideSwitch)return {"1","C","2"};
    if(k==PartKind::RGBLed)return {"R","G","B","K"};
    if(k==PartKind::SevenSegment)return {"a","b","c","d","e","f","g","dp","COM"};
    if(k==PartKind::Battery3V||k==PartKind::Battery9V||k==PartKind::PowerSupply||k==PartKind::Voltmeter)return {"+","-"};
    if(k==PartKind::Breadboard30){std::vector<std::string> names;for(char letter='a';letter<='j';++letter)for(int n=1;n<=30;++n)names.push_back(std::string(1,letter)+std::to_string(n));return names;}
    if(k==PartKind::AnalogSource)return {"OUT","GND"};
    return {"1","2"};
}
std::vector<std::string> CircuitDocument::pins(const CircuitPart& part){
    if(part.kind!=PartKind::Esp8266||part.model.empty()||part.model=="esp8266")return pins(part.kind);
    const auto profile=BoardProfile::get(part.model);
    if(profile.arduino()){std::vector<std::string> names{"5V","3V3","GND"};for(int gpio:profile.gpio)names.push_back(profile.pinName(gpio));names.push_back("GND2");return names;}
    std::vector<std::string> names{"3V3","GND"};for(int gpio:profile.gpio)names.push_back("GPIO"+std::to_string(gpio));names.insert(names.end(),{"A0","GND2","3V3_2"});return names;
}
CircuitPart& CircuitDocument::part(const std::string& id) {for(auto& p:parts_)if(p.id==id)return p;throw std::out_of_range("Unknown component: "+id);}
const CircuitPart& CircuitDocument::part(const std::string& id) const {for(const auto& p:parts_)if(p.id==id)return p;throw std::out_of_range("Unknown component: "+id);}
void CircuitDocument::insertPart(CircuitPart p) {
    if(parts_.size()>=100)throw std::invalid_argument("Maximum 100 components");
    if(p.id.empty()||p.id.size()>80)throw std::invalid_argument("Invalid component identity");
    for(const auto& existing:parts_)if(existing.id==p.id)throw std::invalid_argument("Duplicate component identity");
    if(!std::isfinite(p.x)||!std::isfinite(p.y)||std::abs(p.x)>10000||std::abs(p.y)>10000||!std::isfinite(p.value))throw std::invalid_argument("Invalid component properties");
    if(p.kind==PartKind::Resistor&&(p.value<1||p.value>1e9))throw std::invalid_argument("Resistance outside 1..1e9 ohms");
    if(p.kind==PartKind::AnalogSource&&(p.value<0||p.value>3.3))throw std::invalid_argument("Analog source outside 0..3.3 V");
    if(p.kind==PartKind::Oled&&p.value!=60&&p.value!=61)throw std::invalid_argument("OLED address must be 0x3C or 0x3D");
    const auto& type=componentSpec(p.kind);if(p.value<type.min||p.value>type.max)throw std::invalid_argument("Component value outside model range");
    if(p.rotation<0||p.rotation>=360||p.rotation%90)throw std::invalid_argument("Invalid component rotation");
    parts_.push_back(std::move(p));
}
std::string CircuitDocument::addPart(PartKind kind,double x,double y) {
    std::string id;do{id="part_"+std::to_string(nextPart_++);}while(std::any_of(parts_.begin(),parts_.end(),[&](const auto& p){return p.id==id;}));
    insertPart({id,kind,x,y,componentSpec(kind).initial,false,"#fb586b",{}});return id;
}
void CircuitDocument::removePart(const std::string& id) {
    (void)part(id);std::erase_if(wires_,[&](const auto& w){return w.a.part==id||w.b.part==id;});std::erase_if(parts_,[&](const auto& p){return p.id==id;});
}
void CircuitDocument::validateEndpoint(const Endpoint& e) const {const auto allowed=pins(part(e.part));if(std::find(allowed.begin(),allowed.end(),e.pin)==allowed.end())throw std::invalid_argument("Unknown pin: "+e.pin);}
std::string CircuitDocument::connect(Endpoint a,Endpoint b,std::string color) {
    validateEndpoint(a);validateEndpoint(b);if(a==b)throw std::invalid_argument("Connect two different pins");
    if(wires_.size()>=300)throw std::invalid_argument("Maximum 300 wires");
    for(const auto& w:wires_)if((w.a==a&&w.b==b)||(w.a==b&&w.b==a))throw std::invalid_argument("These pins are already connected");
    const auto id="wire_"+std::to_string(nextWire_++);wires_.push_back({id,std::move(a),std::move(b),std::move(color),{}});return id;
}
void CircuitDocument::removeWire(const std::string& id){auto old=wires_.size();std::erase_if(wires_,[&](const auto& w){return w.id==id;});if(old==wires_.size())throw std::invalid_argument("Unknown wire");}
void CircuitDocument::clearWires(){wires_.clear();}
void CircuitDocument::validate() const {
    if(parts_.size()>100||wires_.size()>300)throw std::invalid_argument("Invalid circuit size");
    std::set<std::string> ids;
    for(const auto& p:parts_){const auto& type=componentSpec(p.kind);if(p.value<type.min||p.value>type.max)throw std::invalid_argument("Invalid component value");if(!ids.insert(p.id).second)throw std::invalid_argument("Duplicate component");if(!std::isfinite(p.x)||!std::isfinite(p.y)||std::abs(p.x)>10000||std::abs(p.y)>10000||!std::isfinite(p.value))throw std::invalid_argument("Invalid component properties");if(p.kind==PartKind::Resistor&&(p.value<1||p.value>1e9))throw std::invalid_argument("Invalid resistance");if(p.kind==PartKind::AnalogSource&&(p.value<0||p.value>3.3))throw std::invalid_argument("Invalid source voltage");if(p.kind==PartKind::Oled&&p.value!=60&&p.value!=61)throw std::invalid_argument("Invalid OLED address");}
    for(const auto& p:parts_){if(p.rotation<0||p.rotation>=360||p.rotation%90)throw std::invalid_argument("Invalid component rotation");if(p.kind==PartKind::Esp8266)(void)BoardProfile::get(p.model);}
    for(const auto& w:wires_){validateEndpoint(w.a);validateEndpoint(w.b);if(w.points.size()>64)throw std::invalid_argument("Maximum 64 wire bends");for(const auto& point:w.points)if(!std::isfinite(point.x)||!std::isfinite(point.y)||std::abs(point.x)>10000||std::abs(point.y)>10000)throw std::invalid_argument("Invalid wire bend");}
}
bool CircuitDocument::connected(const Endpoint& a,const Endpoint& b) const {validateEndpoint(a);validateEndpoint(b);Nodes nodes(*this);return nodes.node(a)==nodes.node(b);}
CircuitState CircuitDocument::solve(const Esp8266Board& board) const {
    validate();Nodes nodes(*this);CircuitState result;std::map<int,double> fixed;std::vector<Branch> resistors;
    auto driver=[&](int n,double v){if(fixed.contains(n)&&std::abs(fixed.at(n)-v)>1e-6)result.errors.push_back("Kurzschluss: widersprüchliche Spannungsquellen auf einem Netz.");else fixed[n]=v;};
    const auto active=boardId();
    const int ground=active.empty()?-1:nodes.node({active,"GND"});
    const int supply=active.empty()?-1:nodes.node({active,board.profile().arduino()?"5V":"3V3"});
    for(const auto& p:parts_)if(p.kind==PartKind::Esp8266){
        const auto profile=BoardProfile::get(p.model);
        for(const auto& pin:pins(p))result.logicVoltage[{p.id,pin}]=profile.voltage;
        driver(nodes.node({p.id,"GND"}),0);driver(nodes.node({p.id,"3V3"}),3.3);
        if(profile.arduino())driver(nodes.node({p.id,"5V"}),5);
    }
    for(const auto& [ep,index]:nodes.index)if(ep.part==active&&board.profile().pinNumber(ep.pin)>=0) {
        const int gpio=board.profile().pinNumber(ep.pin),n=nodes.find(index);const auto& pin=board.pin(gpio);
        if(pin.mode==PinMode::Output)driver(n,pin.output?board.profile().voltage:0);
        else if(pin.mode==PinMode::InputPullup)resistors.push_back({n,supply,47000});
        if(pin.external==Level::High)driver(n,board.profile().voltage);
        if(pin.external==Level::Low)driver(n,0);
        if(pin.external==Level::Contention)result.errors.push_back("GPIO input has conflicting external drivers");
    }
    for(const auto& p:parts_) {
        if(p.kind==PartKind::Oled){resistors.push_back({nodes.node({p.id,"SDA"}),nodes.node({p.id,"VCC"}),4700});resistors.push_back({nodes.node({p.id,"SCL"}),nodes.node({p.id,"VCC"}),4700});}
        if(p.kind==PartKind::Potentiometer){resistors.push_back({nodes.node({p.id,"1"}),nodes.node({p.id,"W"}),std::max(0.01,100*p.value)});resistors.push_back({nodes.node({p.id,"W"}),nodes.node({p.id,"2"}),std::max(0.01,100*(100-p.value))});}
        if(p.kind==PartKind::Photoresistor)resistors.push_back({nodes.node({p.id,"1"}),nodes.node({p.id,"2"}),std::clamp(10000*std::pow(10/std::max(0.01,p.value),0.7),10.0,1e7)});
        if(p.kind==PartKind::Thermistor)resistors.push_back({nodes.node({p.id,"1"}),nodes.node({p.id,"2"}),10000*std::exp(3950*(1/(p.value+273.15)-1/298.15))});
        if((p.kind==PartKind::Battery3V||p.kind==PartKind::Battery9V||p.kind==PartKind::PowerSupply)&&nodes.node({p.id,"-"})==ground)driver(nodes.node({p.id,"+"}),p.value);
        if(p.kind==PartKind::Resistor)resistors.push_back({nodes.node({p.id,"1"}),nodes.node({p.id,"2"}),p.value});
        if(p.kind==PartKind::AnalogSource&&nodes.node({p.id,"GND"})==ground)driver(nodes.node({p.id,"OUT"}),p.value);
    }
    if(!result.errors.empty())return result;
    struct Junction{std::string id;int a,k;double vf,resistance,limit;};std::vector<Junction> junctions;
    for(const auto& p:parts_){if(p.kind==PartKind::Led||p.kind==PartKind::Diode)junctions.push_back({p.id,nodes.node({p.id,"A"}),nodes.node({p.id,"K"}),p.kind==PartKind::Diode?0.7:2.0,p.kind==PartKind::Diode?1.0:10.0,p.kind==PartKind::Diode?1.0:0.02});
        if(p.kind==PartKind::RGBLed)for(const auto& channel:std::vector<std::pair<std::string,double>>{{"R",2},{"G",2.2},{"B",3}})junctions.push_back({p.id+":"+channel.first,nodes.node({p.id,channel.first}),nodes.node({p.id,"K"}),channel.second,10,0.02});
        if(p.kind==PartKind::SevenSegment)for(const auto& channel:std::vector<std::string>{"a","b","c","d","e","f","g","dp"})junctions.push_back({p.id+":"+channel,nodes.node({p.id,channel}),nodes.node({p.id,"COM"}),2,10,0.02});
    }
    std::map<std::string,bool> conducting;std::map<int,double> volts;
    for(int iteration=0;iteration<24;++iteration) {
        auto branches=resistors;
        for(const auto& j:junctions)if(conducting[j.id])branches.push_back({j.a,j.k,j.resistance,j.vf});
        std::set<int> anchored;for(const auto& [n,v]:fixed){(void)v;anchored.insert(n);}
        bool grew=true;while(grew){grew=false;for(const auto& b:branches){if(anchored.contains(b.a)&&anchored.insert(b.b).second)grew=true;if(anchored.contains(b.b)&&anchored.insert(b.a).second)grew=true;}}
        std::map<int,int> unknown;for(int n:anchored)if(!fixed.contains(n))unknown[n]=static_cast<int>(unknown.size());
        std::vector<std::vector<double>> matrix(unknown.size(),std::vector<double>(unknown.size()));std::vector<double> rhs(unknown.size());
        for(const auto& b:branches)if(b.a!=b.b&&anchored.contains(b.a)&&anchored.contains(b.b)) {
            const double g=1/b.resistance;
            for(int side=0;side<2;++side){int a=side?b.b:b.a,c=side?b.a:b.b;double offset=side?-b.offset:b.offset;if(!unknown.contains(a))continue;int row=unknown.at(a);matrix[row][row]+=g;rhs[row]+=g*offset;if(unknown.contains(c))matrix[row][unknown.at(c)]-=g;else rhs[row]+=g*fixed.at(c);}
        }
        const auto solved=linearSolve(std::move(matrix),std::move(rhs));volts=fixed;for(const auto& [n,row]:unknown)volts[n]=solved[row];
        bool changed=false;for(const auto& j:junctions){bool on=volts.contains(j.a)&&volts.contains(j.k)&&volts[j.a]-volts[j.k]>j.vf+1e-8;if(conducting[j.id]!=on){conducting[j.id]=on;changed=true;}}
        if(!changed)break;
        if(iteration==23)result.errors.push_back("LED network did not converge");
    }
    for(const auto& [ep,index]:nodes.index){const int n=nodes.find(index);result.voltage[ep]=volts.contains(n)?std::optional<double>(volts.at(n)):std::nullopt;}
    for(const auto& j:junctions){const double current=volts.contains(j.a)&&volts.contains(j.k)?std::max(0.0,(volts[j.a]-volts[j.k]-j.vf)/j.resistance):0;result.ledCurrent[j.id]=current;if(current>j.limit+1e-6)result.errors.push_back("Überstrom: "+j.id+". Vorwiderstand / Beschaltung prüfen.");}
    for(const auto& ep:result.voltage)if(ep.first.part==active&&ep.second){const double limit=ep.first.pin=="A0"?board.profile().adcMax:board.profile().voltage+0.3;if(*ep.second>limit+1e-6||*ep.second<-0.3)result.errors.push_back("Überspannung am Boardpin "+ep.first.pin);}

    return result;
}
bool CircuitDocument::oledReady(const std::string& id,int sda,int scl,const CircuitState& state) const {
    if(part(id).kind!=PartKind::Oled||sda==scl)return false;
    const auto v=state.voltage.find({id,"VCC"}),g=state.voltage.find({id,"GND"});
    if(v==state.voltage.end()||g==state.voltage.end()||!v->second||!g->second||*v->second-*g->second<3.0||*v->second-*g->second>3.6)return false;
    if(connected({id,"SDA"},{id,"SCL"}))return false;
    if(state.level({id,"SDA"})!=Level::High||state.level({id,"SCL"})!=Level::High)return false;
    const auto active=boardId();if(active.empty())return false;
    const auto profile=BoardProfile::get(part(active).model);
    return connected({id,"GND"},{active,"GND"})&&connected({id,"SDA"},{active,profile.pinName(sda)})&&connected({id,"SCL"},{active,profile.pinName(scl)});
}
}
