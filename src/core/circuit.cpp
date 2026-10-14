#include "thinkercad/circuit.hpp"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>
#include <tuple>
namespace thinkercad {
namespace {
struct Nodes {
    std::map<Endpoint,int> index;
    std::vector<int> parent;
    int find(int a) { return parent[a]==a?a:parent[a]=find(parent[a]); }
    void join(const Endpoint& a,const Endpoint& b) { int x=find(index.at(a)),y=find(index.at(b));if(x!=y)parent[y]=x; }
    explicit Nodes(const CircuitDocument& doc) {
        for(const auto& p:doc.parts())for(const auto& pin:CircuitDocument::pins(p.kind))index.emplace(Endpoint{p.id,pin},static_cast<int>(index.size()));
        parent.resize(index.size());std::iota(parent.begin(),parent.end(),0);
        join({"board","GND"},{"board","GND2"});join({"board","3V3"},{"board","3V3_2"});
        for(const auto& w:doc.wires())join(w.a,w.b);
        for(const auto& p:doc.parts())if(p.kind==PartKind::Button&&p.pressed)join({p.id,"1"},{p.id,"2"});
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
    const auto v=*found->second;if(v<=0.825)return Level::Low;if(v>=2.475)return Level::High;return Level::Floating;
}
CircuitDocument::CircuitDocument() {parts_.push_back({"board",PartKind::Esp8266,80,80,0,false});}
std::vector<std::string> CircuitDocument::pins(PartKind k) {
    if(k==PartKind::Esp8266)return {"3V3","GND","GPIO16","GPIO5","GPIO4","GPIO0","GPIO2","A0","GND2","GPIO14","GPIO12","GPIO13","GPIO15","GPIO3","GPIO1","3V3_2"};
    if(k==PartKind::Oled)return {"GND","VCC","SCL","SDA"};
    if(k==PartKind::Led)return {"A","K"};
    if(k==PartKind::AnalogSource)return {"OUT","GND"};
    return {"1","2"};
}
CircuitPart& CircuitDocument::part(const std::string& id) {for(auto& p:parts_)if(p.id==id)return p;throw std::out_of_range("Unknown component: "+id);}
const CircuitPart& CircuitDocument::part(const std::string& id) const {for(const auto& p:parts_)if(p.id==id)return p;throw std::out_of_range("Unknown component: "+id);}
void CircuitDocument::insertPart(CircuitPart p) {
    if(parts_.size()>=100)throw std::invalid_argument("Maximum 100 components");
    if(p.id.empty()||p.id.size()>80||p.kind==PartKind::Esp8266)throw std::invalid_argument("Invalid component identity");
    for(const auto& existing:parts_)if(existing.id==p.id)throw std::invalid_argument("Duplicate component identity");
    if(!std::isfinite(p.x)||!std::isfinite(p.y)||std::abs(p.x)>10000||std::abs(p.y)>10000||!std::isfinite(p.value))throw std::invalid_argument("Invalid component properties");
    if(p.kind==PartKind::Resistor&&(p.value<1||p.value>1e9))throw std::invalid_argument("Resistance outside 1..1e9 ohms");
    if(p.kind==PartKind::AnalogSource&&(p.value<0||p.value>1))throw std::invalid_argument("Analog source outside 0..1 V");
    if(p.kind==PartKind::Oled&&p.value!=60&&p.value!=61)throw std::invalid_argument("OLED address must be 0x3C or 0x3D");
    if(p.kind!=PartKind::Oled&&p.kind!=PartKind::Led&&p.kind!=PartKind::Resistor&&p.kind!=PartKind::Button&&p.kind!=PartKind::AnalogSource)throw std::invalid_argument("Unknown component type");
    parts_.push_back(std::move(p));
}
std::string CircuitDocument::addPart(PartKind kind,double x,double y) {
    std::string id;do{id="part_"+std::to_string(nextPart_++);}while(std::any_of(parts_.begin(),parts_.end(),[&](const auto& p){return p.id==id;}));
    insertPart({id,kind,x,y,kind==PartKind::AnalogSource?0.5:kind==PartKind::Oled?60:220,false});return id;
}
void CircuitDocument::removePart(const std::string& id) {
    if(id=="board")throw std::invalid_argument("The primary ESP8266 board cannot be deleted");
    (void)part(id);std::erase_if(wires_,[&](const auto& w){return w.a.part==id||w.b.part==id;});std::erase_if(parts_,[&](const auto& p){return p.id==id;});
}
void CircuitDocument::validateEndpoint(const Endpoint& e) const {const auto allowed=pins(part(e.part).kind);if(std::find(allowed.begin(),allowed.end(),e.pin)==allowed.end())throw std::invalid_argument("Unknown pin: "+e.pin);}
std::string CircuitDocument::connect(Endpoint a,Endpoint b,std::string color) {
    validateEndpoint(a);validateEndpoint(b);if(a==b)throw std::invalid_argument("Connect two different pins");
    if(wires_.size()>=300)throw std::invalid_argument("Maximum 300 wires");
    for(const auto& w:wires_)if((w.a==a&&w.b==b)||(w.a==b&&w.b==a))throw std::invalid_argument("These pins are already connected");
    const auto id="wire_"+std::to_string(nextWire_++);wires_.push_back({id,std::move(a),std::move(b),std::move(color)});return id;
}
void CircuitDocument::removeWire(const std::string& id){auto old=wires_.size();std::erase_if(wires_,[&](const auto& w){return w.id==id;});if(old==wires_.size())throw std::invalid_argument("Unknown wire");}
void CircuitDocument::clearWires(){wires_.clear();}
void CircuitDocument::validate() const {
    if(parts_.empty()||parts_.size()>100||wires_.size()>300)throw std::invalid_argument("Invalid circuit size");
    std::set<std::string> ids;
    for(const auto& p:parts_){if(!ids.insert(p.id).second)throw std::invalid_argument("Duplicate component");if(!std::isfinite(p.x)||!std::isfinite(p.y)||std::abs(p.x)>10000||std::abs(p.y)>10000||!std::isfinite(p.value))throw std::invalid_argument("Invalid component properties");if(p.kind==PartKind::Resistor&&(p.value<1||p.value>1e9))throw std::invalid_argument("Invalid resistance");if(p.kind==PartKind::AnalogSource&&(p.value<0||p.value>1))throw std::invalid_argument("Invalid source voltage");if(p.kind==PartKind::Oled&&p.value!=60&&p.value!=61)throw std::invalid_argument("Invalid OLED address");}
    if(part("board").kind!=PartKind::Esp8266)throw std::invalid_argument("Missing primary ESP8266");
    for(const auto& w:wires_){validateEndpoint(w.a);validateEndpoint(w.b);}
}
bool CircuitDocument::connected(const Endpoint& a,const Endpoint& b) const {validateEndpoint(a);validateEndpoint(b);Nodes nodes(*this);return nodes.node(a)==nodes.node(b);}
CircuitState CircuitDocument::solve(const Esp8266Board& board) const {
    validate();Nodes nodes(*this);CircuitState result;std::map<int,double> fixed;std::vector<Branch> resistors;
    auto driver=[&](int n,double v){if(fixed.contains(n)&&std::abs(fixed.at(n)-v)>1e-6)result.errors.push_back("Kurzschluss: widersprüchliche Spannungsquellen auf einem Netz.");else fixed[n]=v;};
    const int ground=nodes.node({"board","GND"}),supply=nodes.node({"board","3V3"});driver(ground,0);driver(supply,3.3);
    for(const auto& [ep,index]:nodes.index)if(ep.part=="board"&&ep.pin.starts_with("GPIO")) {
        const int gpio=std::stoi(ep.pin.substr(4)),n=nodes.find(index);const auto& pin=board.pin(gpio);
        if(pin.mode==PinMode::Output)driver(n,pin.output?3.3:0);
        else if(pin.mode==PinMode::InputPullup)resistors.push_back({n,supply,47000});
        if(pin.external==Level::High)driver(n,3.3);
        if(pin.external==Level::Low)driver(n,0);
        if(pin.external==Level::Contention)result.errors.push_back("GPIO input has conflicting external drivers");
    }
    for(const auto& p:parts_) {
        if(p.kind==PartKind::Oled){resistors.push_back({nodes.node({p.id,"SDA"}),nodes.node({p.id,"VCC"}),4700});resistors.push_back({nodes.node({p.id,"SCL"}),nodes.node({p.id,"VCC"}),4700});}
        if(p.kind==PartKind::Resistor)resistors.push_back({nodes.node({p.id,"1"}),nodes.node({p.id,"2"}),p.value});
        if(p.kind==PartKind::AnalogSource&&nodes.node({p.id,"GND"})==ground)driver(nodes.node({p.id,"OUT"}),p.value);
    }
    if(!result.errors.empty())return result;
    std::map<std::string,bool> conducting;std::map<int,double> volts;
    for(int iteration=0;iteration<24;++iteration) {
        auto branches=resistors;
        for(const auto& p:parts_)if(p.kind==PartKind::Led&&conducting[p.id])branches.push_back({nodes.node({p.id,"A"}),nodes.node({p.id,"K"}),10,2.0});
        std::set<int> anchored;for(const auto& [n,v]:fixed){(void)v;anchored.insert(n);}
        bool grew=true;while(grew){grew=false;for(const auto& b:branches){if(anchored.contains(b.a)&&anchored.insert(b.b).second)grew=true;if(anchored.contains(b.b)&&anchored.insert(b.a).second)grew=true;}}
        std::map<int,int> unknown;for(int n:anchored)if(!fixed.contains(n))unknown[n]=static_cast<int>(unknown.size());
        std::vector<std::vector<double>> matrix(unknown.size(),std::vector<double>(unknown.size()));std::vector<double> rhs(unknown.size());
        for(const auto& b:branches)if(b.a!=b.b&&anchored.contains(b.a)&&anchored.contains(b.b)) {
            const double g=1/b.resistance;
            for(int side=0;side<2;++side){int a=side?b.b:b.a,c=side?b.a:b.b;double offset=side?-b.offset:b.offset;if(!unknown.contains(a))continue;int row=unknown.at(a);matrix[row][row]+=g;rhs[row]+=g*offset;if(unknown.contains(c))matrix[row][unknown.at(c)]-=g;else rhs[row]+=g*fixed.at(c);}
        }
        const auto solved=linearSolve(std::move(matrix),std::move(rhs));volts=fixed;for(const auto& [n,row]:unknown)volts[n]=solved[row];
        bool changed=false;for(const auto& p:parts_)if(p.kind==PartKind::Led){const int a=nodes.node({p.id,"A"}),k=nodes.node({p.id,"K"});bool on=volts.contains(a)&&volts.contains(k)&&volts[a]-volts[k]>2.0+1e-8;if(conducting[p.id]!=on){conducting[p.id]=on;changed=true;}}
        if(!changed)break;
        if(iteration==23)result.errors.push_back("LED network did not converge");
    }
    for(const auto& [ep,index]:nodes.index){const int n=nodes.find(index);result.voltage[ep]=volts.contains(n)?std::optional<double>(volts.at(n)):std::nullopt;}
    for(const auto& p:parts_)if(p.kind==PartKind::Led){const auto a=result.voltage.at({p.id,"A"}),k=result.voltage.at({p.id,"K"});const double current=a&&k?std::max(0.0,(*a-*k-2.0)/10):0;result.ledCurrent[p.id]=current;if(current>0.02+1e-6)result.errors.push_back("LED "+p.id+": Überstrom. Vorwiderstand vergrößern (max. 20 mA im Modell).");}
    return result;
}
bool CircuitDocument::oledReady(const std::string& id,int sda,int scl,const CircuitState& state) const {
    if(part(id).kind!=PartKind::Oled||sda==scl)return false;
    const auto v=state.voltage.find({id,"VCC"}),g=state.voltage.find({id,"GND"});
    if(v==state.voltage.end()||g==state.voltage.end()||!v->second||!g->second||*v->second-*g->second<3.0||*v->second-*g->second>3.6)return false;
    if(connected({id,"SDA"},{id,"SCL"}))return false;
    if(state.level({id,"SDA"})!=Level::High||state.level({id,"SCL"})!=Level::High)return false;
    return connected({id,"GND"},{"board","GND"})&&connected({id,"SDA"},{"board","GPIO"+std::to_string(sda)})&&connected({id,"SCL"},{"board","GPIO"+std::to_string(scl)});
}
}
