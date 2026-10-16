#include "thinkercad/circuit.hpp"
#include "thinkercad/modules.hpp"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>
#include <tuple>
namespace thinkercad {
const std::vector<ComponentSpec>& componentCatalog(){static const std::vector<ComponentSpec> types{
{PartKind::Esp8266,"Mikrocontroller","",0,0,1e9,false},{PartKind::Oled,"OLED SSD1306","",60,60,61,false},{PartKind::Led,"LED","",220,0,1e9,false},{PartKind::Resistor,"Widerstand"," Ω",220,1,1e9,true},{PartKind::Button,"Taster","",220,0,1e9,false},{PartKind::AnalogSource,"Analogquelle"," V",0.5,0,3.3,true},
{PartKind::Potentiometer,"Potentiometer 10 kΩ"," %",50,0,100,true},{PartKind::Photoresistor,"Fotowiderstand"," lx",100,0,100000,true},{PartKind::Thermistor,"NTC 10k B3950"," °C",25,-40,125,true},{PartKind::SlideSwitch,"Schiebeschalter SPDT","",0,0,1,false},{PartKind::TiltSwitch,"Neigungsschalter","",0,0,1,false},{PartKind::ReedSwitch,"Reedkontakt","",0,0,1,false},{PartKind::Diode,"Siliziumdiode","",0,0,1,false},{PartKind::Battery3V,"3-V-Batterie (ideal)","",3,3,3,false},{PartKind::Battery9V,"9-V-Batterie (ideal)","",9,9,9,false},{PartKind::PowerSupply,"Gleichspannungsquelle"," V",5,0,12,true},{PartKind::RGBLed,"RGB-LED (Kathode)","",0,0,1,false},{PartKind::SevenSegment,"7-Segment (Kathode)","",0,0,1,false},{PartKind::Voltmeter,"Voltmeter (ideal)","",0,0,1,false},{PartKind::Breadboard30,"Breadboard 300 Kontakte","",0,0,1,false},
{PartKind::PiezoBuzzer,"Piezo-Summer (passiv)","",0,0,1,false},
{PartKind::Speaker,"Mini-Lautsprecher 8 Ω","",8,8,8,false},
{PartKind::TemperatureSensor,"Temperatursensor TMP36"," °C",25,-40,125,true},
{PartKind::BatteryAA,"AA-Batterie 1,5 V (ideal)","",1.5,1.5,1.5,false},
{PartKind::ToggleSwitch,"Ein-/Ausschalter SPST","",0,0,1,false},
{PartKind::Capacitor,"Kondensator"," µF",10,0.001,1000000,true},
{PartKind::Inductor,"Spule"," mH",10,0.001,100000,true},
{PartKind::NpnTransistor,"NPN-Transistor"," β",100,1,1000,true},
{PartKind::PnpTransistor,"PNP-Transistor"," β",100,1,1000,true},
{PartKind::NMosfet,"N-Kanal-MOSFET"," Vth",2,0.1,10,true},
{PartKind::PMosfet,"P-Kanal-MOSFET"," Vth",2,0.1,10,true},
{PartKind::OpAmp,"Operationsverstärker"," V/V",100000,10,1000000,true},
{PartKind::Ground,"Masse","",0,0,0,false},
{PartKind::FunctionGenerator,"Funktionsgenerator"," Hz",100,0.1,2000,true},
{PartKind::Oscilloscope,"Oszilloskop (2 Kanäle)"," ms/div",1,0.01,1000,true},
{PartKind::LogicAnalyzer,"Logikanalysator (8 Kanäle)"," V Schwelle",2.5,0.1,10,true},
{PartKind::CurrentProbe,"Stromsonde"," A max",10,0.001,100,true},
{PartKind::Junction,"Verbindungspunkt","",0,0,0,false},
{PartKind::NetLabel,"Netzlabel","",0,0,0,false},
{PartKind::CharacterLcd,"LCD HD44780 (4 Bit)","",0,0,0,false},
{PartKind::I2cCharacterLcd,"LCD HD44780 (I²C)"," Adresse",39,8,119,true},
{PartKind::AndGate,"AND","",0,0,0,false},
{PartKind::NandGate,"NAND","",0,0,0,false},
{PartKind::OrGate,"OR","",0,0,0,false},
{PartKind::NorGate,"NOR","",0,0,0,false},
{PartKind::XorGate,"XOR","",0,0,0,false},
{PartKind::XnorGate,"XNOR","",0,0,0,false},
{PartKind::NotGate,"NOT","",0,0,0,false},
{PartKind::ServoMotor,"Servo","",0,0,0,false},
{PartKind::DcMotor,"Gleichstrommotor"," Ω",10,1,1000,true},
{PartKind::InfraredLed,"IR-LED 940 nm","",0,0,0,false},
{PartKind::InfraredReceiver,"IR-Empfänger 38 kHz","",0,0,0,false},
{PartKind::InfraredRemote,"TV-Fernbedienung (NEC)"," Kommando",69,0,255,true},
{PartKind::NfcReader,"NFC-Leser PN532 (I²C)","",36,36,36,false},
{PartKind::NfcTestCard,"NFC-Testkarte","",0,0,0,false},
{PartKind::RadioTransmitter,"433-MHz-Sender (ASK)"," MHz",433.92,300,928,true},
{PartKind::RadioReceiver,"433-MHz-Empfänger (ASK)"," MHz",433.92,300,928,true},
{PartKind::RadioTransceiver,"nRF24L01+ (SPI)"," Kanal",76,0,125,true},
{PartKind::Note,"Notiz","",0,0,0,false},
{PartKind::Module,"Sensormodul","",0,0,1e9,false}
};return types;}
double parameter(const CircuitPart& part,const std::string& name,double fallback){auto found=part.parameters.find(name);return found==part.parameters.end()?fallback:found->second;}
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
        std::map<std::string,Endpoint> named;for(const auto& p:doc.parts())if(p.kind==PartKind::NetLabel&&!p.netName.empty()){auto [it,inserted]=named.emplace(p.netName,Endpoint{p.id,"NET"});if(!inserted)join(it->second,{p.id,"NET"});}
        for(const auto& p:doc.parts()){
            if((p.kind==PartKind::ToggleSwitch||p.kind==PartKind::Button||p.kind==PartKind::TiltSwitch||p.kind==PartKind::ReedSwitch)&&p.pressed)join({p.id,"1"},{p.id,"2"});
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
CircuitDocument::CircuitDocument() {parts_.push_back({"board",PartKind::Esp8266,80,80,0,false,"#fb586b","esp8266",0,{}, {}});}
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
    if(k==PartKind::NpnTransistor||k==PartKind::PnpTransistor)return {"C","B","E"};
    if(k==PartKind::NMosfet||k==PartKind::PMosfet)return {"D","G","S"};
    if(k==PartKind::InfraredLed)return {"A","K"};
    if(k==PartKind::InfraredReceiver)return {"VCC","GND","OUT"};
    if(k==PartKind::Note||k==PartKind::InfraredRemote||k==PartKind::NfcTestCard)return {};
    if(k==PartKind::NfcReader)return {"VCC","GND","SDA","SCL","IRQ","RST"};
    if(k==PartKind::RadioTransmitter||k==PartKind::RadioReceiver)return {"VCC","GND","DATA"};
    if(k==PartKind::RadioTransceiver)return {"VCC","GND","SCK","MISO","MOSI","CSN","CE","IRQ"};
    if(k==PartKind::ServoMotor)return {"GND","VCC","SIGNAL"};
    if(k==PartKind::DcMotor)return {"+","-"};
    if(k>=PartKind::AndGate&&k<=PartKind::NotGate)return k==PartKind::NotGate?std::vector<std::string>{"A","Y","VCC","GND"}:std::vector<std::string>{"A","B","Y","VCC","GND"};
    if(k==PartKind::CharacterLcd)return {"GND","VCC","VO","RS","RW","E","D0","D1","D2","D3","D4","D5","D6","D7","A","K"};
    if(k==PartKind::I2cCharacterLcd)return {"GND","VCC","SDA","SCL"};
    if(k==PartKind::Junction||k==PartKind::NetLabel)return {"NET"};
    if(k==PartKind::OpAmp)return {"IN+","IN-","V+","V-","OUT"};
    if(k==PartKind::Ground)return {"GND"};
    if(k==PartKind::FunctionGenerator)return {"OUT","GND"};
    if(k==PartKind::Oscilloscope)return {"A","B","GND"};
    if(k==PartKind::LogicAnalyzer)return {"D0","D1","D2","D3","D4","D5","D6","D7","GND"};
    if(k==PartKind::CurrentProbe)return {"+","-"};
    if(k==PartKind::Oled)return {"GND","VCC","SCL","SDA"};
    if(k==PartKind::Led||k==PartKind::Diode)return {"A","K"};
    if(k==PartKind::TemperatureSensor)return {"VCC","OUT","GND"};
    if(k==PartKind::PiezoBuzzer||k==PartKind::Speaker||k==PartKind::BatteryAA)return {"+","-"};
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
    if(part.kind==PartKind::Module)return ModuleRegistry::get(part.model.empty()?"bme280":part.model).pins;
    if(part.kind!=PartKind::Esp8266||part.model.empty()||part.model=="esp8266")return pins(part.kind);
    const auto profile=BoardProfile::get(part.model);
    if(profile.arduino()){std::vector<std::string> names{"5V","3V3","GND"};for(int gpio:profile.gpio)names.push_back(profile.pinName(gpio));names.push_back("GND2");return names;}
    std::vector<std::string> names{"3V3","GND"};for(int gpio:profile.gpio)names.push_back(profile.pinName(gpio));names.insert(names.end(),{"A0","GND2","3V3_2"});return names;
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
    if(p.kind==PartKind::Module){if(p.model.empty())p.model="bme280";const auto& definition=ModuleRegistry::get(p.model);for(const auto& param:definition.parameters){const auto value=parameter(p,param.name,param.initial);if(!std::isfinite(value)||value<param.min||value>param.max)throw std::invalid_argument("Module parameter outside range: "+param.name);}}
    parts_.push_back(std::move(p));
}
std::string CircuitDocument::addPart(PartKind kind,double x,double y) {
    std::string id;do{id="part_"+std::to_string(nextPart_++);}while(std::any_of(parts_.begin(),parts_.end(),[&](const auto& p){return p.id==id;}));
    insertPart({id,kind,x,y,componentSpec(kind).initial,kind==PartKind::NfcTestCard,"#fb586b",{},0,{}, {}});return id;
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
    for(const auto& p:parts_){
        if(p.netName.size()>48||(!p.netName.empty()&&p.kind!=PartKind::NetLabel)||std::any_of(p.netName.begin(),p.netName.end(),[](unsigned char c){return !((c>='A'&&c<='Z')||(c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='_'||c=='-'||c=='.');}))throw std::invalid_argument("Invalid net label (use A-Z, a-z, 0-9, _, - and .; max 48)");
        if(p.parameters.size()>(p.kind==PartKind::Module?32u:16u))throw std::invalid_argument("Too many component parameters");
        for(const auto& [key,value]:p.parameters){
            if(!std::isfinite(value))throw std::invalid_argument("Non-finite component parameter");
            bool valid=p.kind==PartKind::Note&&(key=="noteWidth"||key=="noteHeight")&&value>=32&&value<=4096;
            valid|=p.kind==PartKind::FunctionGenerator&&((key=="amplitude"&&value>=0&&value<=25)||(key=="offset"&&std::abs(value)<=25)||(key=="duty"&&value>=0.01&&value<=0.99)||(key=="waveform"&&value>=0&&value<=3&&std::floor(value)==value));
            valid|=p.kind==PartKind::Inductor&&key=="resistance"&&value>=0.001&&value<=100000;
            valid|=(p.kind==PartKind::NMosfet||p.kind==PartKind::PMosfet)&&key=="k"&&value>=0.00001&&value<=10;
            valid|=p.kind==PartKind::Resistor&&key=="powerRating"&&value>=.01&&value<=1000;
            valid|=p.kind==PartKind::DcMotor&&((key=="loadTorque"&&value>=0&&value<=1)||(key=="inertia"&&value>=1e-7&&value<=1)||(key=="inductance"&&value>=.001&&value<=1000));
            valid|=p.kind>=PartKind::InfraredLed&&p.kind<=PartKind::RadioTransceiver&&key=="channel"&&value>=0&&value<=15&&std::floor(value)==value;
            valid|=p.kind==PartKind::InfraredRemote&&key=="address"&&value>=0&&value<=65535&&std::floor(value)==value;
            valid|=p.kind==PartKind::NfcTestCard&&((key=="uidLength"&&(value==4||value==7))||(key.size()==4&&key.substr(0,3)=="uid"&&key[3]>='0'&&key[3]<='6'&&value>=0&&value<=255&&std::floor(value)==value));
            if(p.kind==PartKind::Module){const auto& model=ModuleRegistry::get(p.model);for(const auto& field:model.parameters)if(field.name==key&&value>=field.min&&value<=field.max)valid=true;valid|=key=="address"&&model.bus=="i2c"&&value>=(model.builtin?model.address:8)&&value<=(model.builtin?model.address+1:119)&&std::floor(value)==value;}
            if(!valid)throw std::invalid_argument("Invalid component parameter: "+key);
        }
    }
    for(const auto& p:parts_){if(p.kind==PartKind::Note&&p.model.size()>4000)throw std::invalid_argument("Note exceeds 4000 UTF-8 bytes");if(p.rotation<0||p.rotation>=360||p.rotation%90)throw std::invalid_argument("Invalid component rotation");if(p.kind==PartKind::NfcTestCard&&p.model.size()>96)throw std::invalid_argument("NFC test text exceeds 96 bytes");if((p.kind==PartKind::InfraredRemote||p.kind==PartKind::RadioTransceiver)&&std::floor(p.value)!=p.value)throw std::invalid_argument("IR command must be an integer");if(p.kind==PartKind::Esp8266)(void)BoardProfile::get(p.model);if(p.kind==PartKind::Module)(void)ModuleRegistry::get(p.model);}
    for(const auto& w:wires_){validateEndpoint(w.a);validateEndpoint(w.b);if(w.points.size()>64)throw std::invalid_argument("Maximum 64 wire bends");for(const auto& point:w.points)if(!std::isfinite(point.x)||!std::isfinite(point.y)||std::abs(point.x)>10000||std::abs(point.y)>10000)throw std::invalid_argument("Invalid wire bend");}
}
bool CircuitDocument::connected(const Endpoint& a,const Endpoint& b) const {validateEndpoint(a);validateEndpoint(b);Nodes nodes(*this);return nodes.node(a)==nodes.node(b);}
CircuitState CircuitDocument::solve(const Esp8266Board& board) const {
    return solve(std::map<std::string,const Esp8266Board*>{{boardId(),&board}});
}
CircuitState CircuitDocument::solve(const std::map<std::string,const Esp8266Board*>& boards)const {
    return solveStep(boards,nullptr,0,0);
}
CircuitState CircuitDocument::solveStep(const std::map<std::string,const Esp8266Board*>& boards,const ReactiveState* memory,double dt,double time)const {
    validate();Nodes nodes(*this);CircuitState result;std::map<int,double> fixed;std::vector<Branch> resistors;
    auto driver=[&](int n,double v){if(fixed.contains(n)&&std::abs(fixed.at(n)-v)>1e-6){std::string endpoints;for(const auto& [ep,index]:nodes.index)if(nodes.find(index)==n)endpoints+=" "+ep.part+"."+ep.pin;result.errors.push_back("Kurzschluss: widersprüchliche Spannungsquellen auf einem Netz."+endpoints);}else fixed[n]=v;};
    std::set<int> grounds;for(const auto& part:parts_)if(part.kind==PartKind::Esp8266)grounds.insert(nodes.node({part.id,"GND"}));
    for(const auto& p:parts_)if(p.kind==PartKind::Ground){driver(nodes.node({p.id,"GND"}),0);grounds.insert(nodes.node({p.id,"GND"}));}
    for(const auto& p:parts_)if(p.kind==PartKind::Esp8266){
        const auto profile=BoardProfile::get(p.model);
        for(const auto& pin:pins(p))result.logicVoltage[{p.id,pin}]=profile.voltage;
        driver(nodes.node({p.id,"GND"}),0);driver(nodes.node({p.id,"3V3"}),3.3);
        if(profile.arduino())driver(nodes.node({p.id,"5V"}),5);
    }
    for(const auto& [id,source]:boards){if(!source)continue;const auto& board=*source;
    for(const auto& [ep,index]:nodes.index)if(ep.part==id&&board.profile().pinNumber(ep.pin)>=0) {
        const int gpio=board.profile().pinNumber(ep.pin),n=nodes.find(index);const auto& pin=board.pin(gpio);
        if(pin.mode==PinMode::Output){
            double voltage=pin.output?board.profile().voltage:0;
            if(pin.frequency&&memory){const double phase=time*pin.frequency-std::floor(time*pin.frequency);voltage=phase<pin.duty?board.profile().voltage:0;}
            else if(pin.pwm)voltage=pin.duty*board.profile().voltage;
            driver(n,voltage);
        }
        else if(pin.mode==PinMode::InputPullup)resistors.push_back({n,nodes.node({id,board.profile().arduino()?"5V":"3V3"}),47000});
        if(pin.external==Level::High)driver(n,board.profile().voltage);
        if(pin.external==Level::Low)driver(n,0);
        if(pin.external==Level::Contention)result.errors.push_back("GPIO input has conflicting external drivers");
    }
    }
    for(const auto& p:parts_) {
        if(p.kind==PartKind::Module){const auto& model=ModuleRegistry::get(p.model);if(model.idleCurrent>0)resistors.push_back({nodes.node({p.id,model.power}),nodes.node({p.id,model.ground}),3.3/model.idleCurrent});for(const auto& [pin,resistance]:model.pullups)resistors.push_back({nodes.node({p.id,pin}),nodes.node({p.id,model.power}),resistance});}
        if(p.kind==PartKind::ServoMotor)resistors.push_back({nodes.node({p.id,"VCC"}),nodes.node({p.id,"GND"}),1000});
        if(p.kind==PartKind::DcMotor){double companion=memory?parameter(p,"inductance",1)*.001/dt:0,current=memory&&memory->motorCurrent.contains(p.id)?memory->motorCurrent.at(p.id):0,speed=memory&&memory->motorSpeed.contains(p.id)?memory->motorSpeed.at(p.id):0;resistors.push_back({nodes.node({p.id,"+"}),nodes.node({p.id,"-"}),p.value+companion,.02*speed-companion*current});}
        if(p.kind==PartKind::Capacitor&&memory){
            const auto old=memory->capacitorVoltage.find(p.id);
            resistors.push_back({nodes.node({p.id,"1"}),nodes.node({p.id,"2"}),dt/(p.value*1e-6),old==memory->capacitorVoltage.end()?0:old->second});
        }
        if(p.kind==PartKind::Inductor){const double r=parameter(p,"resistance",0.1);const double companion=memory?p.value*0.001/dt:0;const auto old=memory?memory->inductorCurrent.find(p.id):std::map<std::string,double>::const_iterator{};
            resistors.push_back({nodes.node({p.id,"1"}),nodes.node({p.id,"2"}),r+companion,memory&&old!=memory->inductorCurrent.end()?-companion*old->second:0});
        }
        if(p.kind==PartKind::CurrentProbe)resistors.push_back({nodes.node({p.id,"+"}),nodes.node({p.id,"-"}),0.001});
        if(p.kind==PartKind::FunctionGenerator&&grounds.contains(nodes.node({p.id,"GND"}))){
            const double amplitude=parameter(p,"amplitude",2.5),offset=parameter(p,"offset",2.5),duty=parameter(p,"duty",0.5);
            const double phase=time*p.value-std::floor(time*p.value);const int shape=int(parameter(p,"waveform",0));
            const double wave=shape==1?(phase<duty?1:-1):shape==2?1-4*std::abs(phase-0.5):shape==3?2*phase-1:std::sin(6.283185307179586*phase);
            driver(nodes.node({p.id,"OUT"}),offset+amplitude*wave);
        }
        if(p.kind==PartKind::Speaker||p.kind==PartKind::PiezoBuzzer)
            resistors.push_back({nodes.node({p.id,"+"}),nodes.node({p.id,"-"}),p.kind==PartKind::Speaker?8.0:1e6});
        if(p.kind==PartKind::Oled||p.kind==PartKind::I2cCharacterLcd||p.kind==PartKind::NfcReader){resistors.push_back({nodes.node({p.id,"SDA"}),nodes.node({p.id,"VCC"}),4700});resistors.push_back({nodes.node({p.id,"SCL"}),nodes.node({p.id,"VCC"}),4700});}
        if(p.kind==PartKind::Potentiometer){resistors.push_back({nodes.node({p.id,"1"}),nodes.node({p.id,"W"}),std::max(0.01,100*p.value)});resistors.push_back({nodes.node({p.id,"W"}),nodes.node({p.id,"2"}),std::max(0.01,100*(100-p.value))});}
        if(p.kind==PartKind::Photoresistor)resistors.push_back({nodes.node({p.id,"1"}),nodes.node({p.id,"2"}),std::clamp(10000*std::pow(10/std::max(0.01,p.value),0.7),10.0,1e7)});
        if(p.kind==PartKind::Thermistor)resistors.push_back({nodes.node({p.id,"1"}),nodes.node({p.id,"2"}),10000*std::exp(3950*(1/(p.value+273.15)-1/298.15))});
        if((p.kind==PartKind::BatteryAA||p.kind==PartKind::Battery3V||p.kind==PartKind::Battery9V||p.kind==PartKind::PowerSupply)&&grounds.contains(nodes.node({p.id,"-"})))driver(nodes.node({p.id,"+"}),p.value);
        if(p.kind==PartKind::Resistor)resistors.push_back({nodes.node({p.id,"1"}),nodes.node({p.id,"2"}),p.value});
        if(p.kind==PartKind::AnalogSource&&grounds.contains(nodes.node({p.id,"GND"})))driver(nodes.node({p.id,"OUT"}),p.value);
    }
    if(!result.errors.empty())return result;
    struct Junction{std::string id;int a,k;double vf,resistance,limit;};std::vector<Junction> junctions;
    for(const auto& p:parts_){if(p.kind==PartKind::Led||p.kind==PartKind::Diode||p.kind==PartKind::InfraredLed)junctions.push_back({p.id,nodes.node({p.id,"A"}),nodes.node({p.id,"K"}),p.kind==PartKind::Diode?0.7:p.kind==PartKind::InfraredLed?1.2:2.0,p.kind==PartKind::Diode?1.0:10.0,p.kind==PartKind::Diode?1.0:p.kind==PartKind::InfraredLed?.05:0.02});
        if(p.kind==PartKind::RGBLed)for(const auto& channel:std::vector<std::pair<std::string,double>>{{"R",2},{"G",2.2},{"B",3}})junctions.push_back({p.id+":"+channel.first,nodes.node({p.id,channel.first}),nodes.node({p.id,"K"}),channel.second,10,0.02});
        if(p.kind==PartKind::SevenSegment)for(const auto& channel:std::vector<std::string>{"a","b","c","d","e","f","g","dp"})junctions.push_back({p.id+":"+channel,nodes.node({p.id,channel}),nodes.node({p.id,"COM"}),2,10,0.02});
    }
    std::map<std::string,bool> conducting;std::map<int,std::string> logicOwners;std::map<int,double> volts=fixed;std::set<std::string> poweredSensors;
    const bool nonlinear=std::any_of(parts_.begin(),parts_.end(),[](const auto& p){return p.kind==PartKind::NpnTransistor||p.kind==PartKind::PnpTransistor||p.kind==PartKind::NMosfet||p.kind==PartKind::PMosfet||p.kind==PartKind::OpAmp;});
    for(int iteration=0;iteration<(nonlinear?400:124);++iteration) {
        auto branches=resistors;std::set<std::string> activeAmps;
        for(const auto& j:junctions)if(conducting[j.id])branches.push_back({j.a,j.k,j.resistance,j.vf});
        for(const auto& p:parts_)if(p.kind==PartKind::NpnTransistor||p.kind==PartKind::PnpTransistor||p.kind==PartKind::NMosfet||p.kind==PartKind::PMosfet){
            const auto terminals=pins(p);for(int i=0;i<2;++i)branches.push_back({nodes.node({p.id,terminals[i]}),nodes.node({p.id,terminals[2]}),1e12});
        }else if(p.kind==PartKind::OpAmp){const int hi=nodes.node({p.id,"V+"}),lo=nodes.node({p.id,"V-"});if(volts.contains(hi)&&volts.contains(lo)&&volts.at(hi)-volts.at(lo)>.1){branches.push_back({nodes.node({p.id,"OUT"}),lo,50});activeAmps.insert(p.id);}}
        std::set<int> anchored;for(const auto& [n,v]:fixed){(void)v;anchored.insert(n);}
        bool grew=true;while(grew){grew=false;for(const auto& b:branches){if(anchored.contains(b.a)&&anchored.insert(b.b).second)grew=true;if(anchored.contains(b.b)&&anchored.insert(b.a).second)grew=true;}}
        std::map<int,int> unknown;for(int n:anchored)if(!fixed.contains(n))unknown[n]=static_cast<int>(unknown.size());
        std::vector<std::vector<double>> matrix(unknown.size(),std::vector<double>(unknown.size()));std::vector<double> rhs(unknown.size());
        for(const auto& b:branches)if(b.a!=b.b&&anchored.contains(b.a)&&anchored.contains(b.b)) {
            const double g=1/b.resistance;
            for(int side=0;side<2;++side){int a=side?b.b:b.a,c=side?b.a:b.b;double offset=side?-b.offset:b.offset;if(!unknown.contains(a))continue;int row=unknown.at(a);matrix[row][row]+=g;rhs[row]+=g*offset;if(unknown.contains(c))matrix[row][unknown.at(c)]-=g;else rhs[row]+=g*fixed.at(c);}
        }
        if(nonlinear&&iteration==0){
            // Start high-impedance control terminals at their passive bias, rather
            // than zero. Limit only floating BJT base guesses before Newton.
            const auto passive=linearSolve(matrix,rhs);for(const auto& [n,row]:unknown)volts[n]=passive[row];
            for(const auto& p:parts_)if(p.kind==PartKind::NpnTransistor||p.kind==PartKind::PnpTransistor){const int b=nodes.node({p.id,"B"}),e=nodes.node({p.id,"E"});if(unknown.contains(b)&&volts.contains(e)){const double sign=p.kind==PartKind::NpnTransistor?1:-1;volts[b]=volts[e]+sign*std::clamp(sign*(volts[b]-volts[e]),-.6,.6);}}
        }
        // Newton linearization stamps device terminal currents and their Jacobians.
        auto value=[&](int node){return volts.contains(node)?volts.at(node):0.0;};
        std::vector<std::pair<std::vector<int>,std::function<std::vector<double>(std::vector<double>)>>> devices;
        auto stamp=[&](const std::vector<int>& terminals,const std::function<std::vector<double>(std::vector<double>)>& currents){
            devices.emplace_back(terminals,currents);
            std::vector<double> guess;for(int n:terminals)guess.push_back(value(n));const auto base=currents(guess);
            std::vector<std::vector<double>> jac(terminals.size(),std::vector<double>(terminals.size()));
            for(std::size_t col=0;col<terminals.size();++col){auto probe=guess;probe[col]+=1e-6;const auto next=currents(probe);for(std::size_t row=0;row<base.size();++row)jac[row][col]=(next[row]-base[row])/1e-6;}
            for(std::size_t r=0;r<terminals.size();++r)if(unknown.contains(terminals[r])){
                const int row=unknown.at(terminals[r]);double affine=base[r];
                for(std::size_t c=0;c<terminals.size();++c){affine-=jac[r][c]*guess[c];if(unknown.contains(terminals[c]))matrix[row][unknown.at(terminals[c])]+=jac[r][c];else if(fixed.contains(terminals[c]))rhs[row]-=jac[r][c]*fixed.at(terminals[c]);}
                rhs[row]-=affine;
            }
        };
        for(const auto& p:parts_){
            if(p.kind==PartKind::NpnTransistor||p.kind==PartKind::PnpTransistor){
                stamp({nodes.node({p.id,"C"}),nodes.node({p.id,"B"}),nodes.node({p.id,"E"})},[p](const auto& v){
                    const double sign=p.kind==PartKind::NpnTransistor?1:-1;
                    auto diode=[](double voltage){return 1e-14*std::expm1(std::clamp(voltage/0.02585,-40.0,40.0));};
                    const double be=diode(sign*(v[1]-v[2])),bc=diode(sign*(v[1]-v[0]));
                    const double collector=sign*(p.value/(p.value+1)*be-bc),emitter=sign*(-be+0.5*bc);
                    return std::vector<double>{collector,-collector-emitter,emitter};
                });
            }else if(p.kind==PartKind::NMosfet||p.kind==PartKind::PMosfet){
                stamp({nodes.node({p.id,"D"}),nodes.node({p.id,"G"}),nodes.node({p.id,"S"})},[p](const auto& v){
                    const double sign=p.kind==PartKind::NMosfet?1:-1;const bool reverse=sign*(v[0]-v[2])<0;
                    const double vd=std::abs(v[0]-v[2]),vg=sign*(v[1]-(reverse?v[0]:v[2])),over=vg-p.value,k=parameter(p,"k",0.02);
                    double current=over<=0?0:vd<over?k*(over*vd-0.5*vd*vd):0.5*k*over*over;
                    current*=1+0.02*vd;current*=sign*(reverse?-1:1);
                    const double body=std::max(0.0,sign*(v[2]-v[0])-0.7);
                    current-=sign*body;return std::vector<double>{current,0,-current};
                });
            }else if(p.kind==PartKind::OpAmp){
                const int hi=nodes.node({p.id,"V+"}),lo=nodes.node({p.id,"V-"});
                if(!activeAmps.contains(p.id)||!anchored.contains(hi)||!anchored.contains(lo)||value(hi)-value(lo)<0.1)continue;
                stamp({nodes.node({p.id,"OUT"}),nodes.node({p.id,"IN+"}),nodes.node({p.id,"IN-"}),lo,hi},[p](const auto& v){
                    const double target=std::clamp(p.value*(v[1]-v[2]),v[3]+0.05,v[4]-0.05),current=(v[0]-target)/50;
                    // The output resistor was already stamped above; subtract its current here.
                    const double extra=current-(v[0]-v[3])/50;
                    return std::vector<double>{extra,0,0,-extra,0};
                });
            }
        }
        const auto previous=volts;const auto solved=linearSolve(std::move(matrix),std::move(rhs));volts=fixed;bool changed=false;
        double alpha=1,residualSquared=0;
        if(nonlinear){
            double largest=0;for(const auto& [n,row]:unknown)largest=std::max(largest,std::abs(solved[row]-(previous.contains(n)?previous.at(n):0)));if(largest>.25)alpha=.25/largest;
            auto residual=[&](const std::map<int,double>& trial){std::map<int,double> currents;auto v=[&](int n){return trial.contains(n)?trial.at(n):0;};for(const auto& b:branches)if(anchored.contains(b.a)&&anchored.contains(b.b)){double i=(v(b.a)-v(b.b)-b.offset)/b.resistance;currents[b.a]+=i;currents[b.b]-=i;}for(const auto& [terminals,model]:devices){std::vector<double> values;for(int n:terminals)values.push_back(v(n));auto i=model(values);for(std::size_t k=0;k<terminals.size();++k)currents[terminals[k]]+=i[k];}double norm=0;for(const auto& [n,row]:unknown){(void)row;norm+=currents[n]*currents[n];}return norm;};
            const double base=residual(previous);
            for(int search=0;search<24;++search){auto trial=fixed;for(const auto& [n,row]:unknown){double old=previous.contains(n)?previous.at(n):0;trial[n]=old+alpha*(solved[row]-old);}residualSquared=residual(trial);if(residualSquared<=base+1e-24)break;alpha*=.5;}
        }
        if(nonlinear&&residualSquared>1e-16)changed=true;
        for(const auto& [n,row]:unknown){const double old=previous.contains(n)?previous.at(n):0;const double next=old+alpha*(solved[row]-old);volts[n]=next;if(nonlinear&&std::abs(next-old)>1e-8)changed=true;}
        for(const auto& j:junctions){bool on=volts.contains(j.a)&&volts.contains(j.k)&&volts[j.a]-volts[j.k]>j.vf+1e-8;if(conducting[j.id]!=on){conducting[j.id]=on;changed=true;}}
        // TMP36: ideal high-impedance sensor, 500 mV offset + 10 mV/°C.
        // Wait for the supply network to resolve, including series resistors/switches.
        for(const auto& p:parts_)if(p.kind==PartKind::TemperatureSensor&&!poweredSensors.contains(p.id)){
            const int v=nodes.node({p.id,"VCC"}),g=nodes.node({p.id,"GND"});
            if(volts.contains(v)&&volts.contains(g)){
                const double supply=volts.at(v)-volts.at(g);
                if(supply< -0.3||supply>5.5)result.errors.push_back("TMP36: Versorgung außerhalb 2,7–5,5 V / Polarität prüfen.");
                if(supply>=2.7&&supply<=5.5){driver(nodes.node({p.id,"OUT"}),volts.at(g)+0.5+0.01*p.value);poweredSensors.insert(p.id);changed=true;}
            }
        }
        for(const auto& p:parts_)if(p.kind>=PartKind::AndGate&&p.kind<=PartKind::NotGate){
            const int v=nodes.node({p.id,"VCC"}),g=nodes.node({p.id,"GND"}),a=nodes.node({p.id,"A"}),out=nodes.node({p.id,"Y"});
            if(!volts.contains(v)||!volts.contains(g))continue;
            double supply=volts.at(v)-volts.at(g);if(supply<1.8||supply>5.5)continue;
            auto input=[&](int n)->std::optional<bool>{if(!volts.contains(n)&&!fixed.contains(n))return {};double value=(fixed.contains(n)?fixed.at(n):volts.at(n))-volts.at(g);if(value<=.3*supply)return false;if(value>=.7*supply)return true;return {};};
            auto aa=input(a),bb=p.kind==PartKind::NotGate?std::optional<bool>(false):input(nodes.node({p.id,"B"}));if(!aa||!bb)continue;
            bool value=p.kind==PartKind::AndGate||p.kind==PartKind::NandGate?*aa&&*bb:p.kind==PartKind::OrGate||p.kind==PartKind::NorGate?*aa||*bb:p.kind==PartKind::NotGate?!*aa:*aa!=*bb;if(p.kind==PartKind::NandGate||p.kind==PartKind::NorGate||p.kind==PartKind::XnorGate)value=!value;
            const double output=volts.at(g)+(value?supply:0);if(!fixed.contains(out)){driver(out,output);logicOwners[out]=p.id;changed=true;}else if(std::abs(fixed.at(out)-output)>1e-6){if(logicOwners.contains(out)&&logicOwners.at(out)==p.id){fixed[out]=output;changed=true;}else result.errors.push_back("Logic gate output conflict: "+p.id);}
        }
        for(const auto& p:parts_)if(p.kind==PartKind::OpAmp){const int hi=nodes.node({p.id,"V+"}),lo=nodes.node({p.id,"V-"});const bool powered=volts.contains(hi)&&volts.contains(lo)&&volts.at(hi)-volts.at(lo)>.1;if(powered!=activeAmps.contains(p.id))changed=true;}
        if(!changed)break;
        if(iteration==(nonlinear?399:123))result.errors.push_back("Electrical network did not converge; check bias, feedback and supply rails.");
    }
    for(const auto& [ep,index]:nodes.index){const int n=nodes.find(index);result.voltage[ep]=volts.contains(n)?std::optional<double>(volts.at(n)):std::nullopt;}
    for(const auto& p:parts_)if(p.kind==PartKind::OpAmp){const auto hi=result.voltage.at({p.id,"V+"}),lo=result.voltage.at({p.id,"V-"});if(hi&&lo&&*hi-*lo<0)result.errors.push_back("Op-amp supply rails reversed.");if(hi&&lo&&*hi-*lo>.1&&(!result.voltage.at({p.id,"IN+"})||!result.voltage.at({p.id,"IN-"})))result.errors.push_back("Op-amp input is floating; connect both inputs and check feedback.");}
    for(const auto& j:junctions){const double current=volts.contains(j.a)&&volts.contains(j.k)?std::max(0.0,(volts[j.a]-volts[j.k]-j.vf)/j.resistance):0;result.ledCurrent[j.id]=current;if(current>j.limit+1e-6)result.errors.push_back("Überstrom: "+j.id+". Vorwiderstand / Beschaltung prüfen.");}
    for(const auto& p:parts_){
        std::string a,b;double resistance=0,offset=0;
        if(p.kind==PartKind::ServoMotor){auto v=result.voltage.at({p.id,"VCC"}),g=result.voltage.at({p.id,"GND"});if(v&&g&&*v-*g>=4.5&&*v-*g<=6&&memory&&memory->servoPulse.contains(p.id))result.partValues[p.id]=std::clamp((memory->servoPulse.at(p.id)*1e6-544)*180/1856,0.,180.);}
        if(p.kind==PartKind::DcMotor){a="+";b="-";double companion=memory?parameter(p,"inductance",1)*.001/dt:0,current=memory&&memory->motorCurrent.contains(p.id)?memory->motorCurrent.at(p.id):0,speed=memory&&memory->motorSpeed.contains(p.id)?memory->motorSpeed.at(p.id):0;resistance=p.value+companion;offset=.02*speed-companion*current;result.partValues[p.id]=speed*60/(2*3.141592653589793);}
        else if(p.kind==PartKind::Resistor){a="1";b="2";resistance=p.value;}
        else if(p.kind==PartKind::CurrentProbe){a="+";b="-";resistance=0.001;}
        else if(p.kind==PartKind::Capacitor){a="1";b="2";if(memory){resistance=dt/(p.value*1e-6);auto old=memory->capacitorVoltage.find(p.id);offset=old==memory->capacitorVoltage.end()?0:old->second;}}
        else if(p.kind==PartKind::Inductor){a="1";b="2";const double companion=memory?p.value*0.001/dt:0;resistance=parameter(p,"resistance",0.1)+companion;if(memory){auto old=memory->inductorCurrent.find(p.id);if(old!=memory->inductorCurrent.end())offset=-companion*old->second;}}
        if(!a.empty()){
            const auto& va=result.voltage.at({p.id,a});const auto& vb=result.voltage.at({p.id,b});
            result.branchCurrent[p.id]=va&&vb?(resistance>0?(*va-*vb-offset)/resistance:0):std::optional<double>{};
            if(p.kind==PartKind::CurrentProbe&&result.branchCurrent[p.id]&&std::abs(*result.branchCurrent[p.id])>p.value)result.errors.push_back("Current probe range exceeded; connect it in series, not across a supply.");
        }
    }
    for(const auto& p:parts_){
        if(p.kind==PartKind::Resistor&&result.branchCurrent.contains(p.id)&&result.branchCurrent.at(p.id)){double current=*result.branchCurrent.at(p.id),power=current*current*p.value,rating=parameter(p,"powerRating",.25);if(power>rating)result.warnings.push_back({p.id,"Resistor dissipates "+std::to_string(power)+" W; rating "+std::to_string(rating)+" W."});}
        if(p.kind>=PartKind::AndGate&&p.kind<=PartKind::NotGate){auto v=result.voltage.at({p.id,"VCC"}),g=result.voltage.at({p.id,"GND"});if(v&&g&&*v-*g>=1.8&&!result.voltage.at({p.id,"Y"}))result.warnings.push_back({p.id,"Logic output is floating: check input voltages and supply (1.8..5.5 V)."});}
    }
    // Check every output load, including motor and reactive companion branches.
    for(const auto& [id,source]:boards)if(source)for(int gpio:source->profile().gpio){
        const auto& pin=source->pin(gpio);if(pin.mode!=PinMode::Output)continue;
        const int n=nodes.node({id,source->profile().pinName(gpio)});double current=0;
        for(const auto& branch:resistors)if(branch.a!=branch.b&&volts.contains(branch.a)&&volts.contains(branch.b)){
            const double flow=(volts.at(branch.a)-volts.at(branch.b)-branch.offset)/branch.resistance;
            if(branch.a==n)current+=flow;
            if(branch.b==n)current-=flow;
        }
        if(std::abs(current)>(source->profile().arduino()?0.04:0.012)+1e-6)
            result.errors.push_back("GPIO-Überstrom: "+id+"."+source->profile().pinName(gpio)+". Last benötigt Vorwiderstand oder Treiber / Verstärker.");
    }
    for(const auto& [id,source]:boards){if(!source)continue;const auto& board=*source;for(const auto& ep:result.voltage)if(ep.first.part==id&&ep.second){const double limit=ep.first.pin=="A0"?board.profile().adcMax:board.profile().voltage+0.3;if(*ep.second>limit+1e-6||*ep.second<-0.3)result.errors.push_back("Überspannung am Boardpin "+id+"."+ep.first.pin);}

    }
    return result;
}
std::vector<SoundSignal> CircuitDocument::soundSignals(const std::map<std::string,const Esp8266Board*>& boards,std::vector<std::string>* errors)const {
    std::vector<SoundSignal> signals;
    if(std::none_of(parts_.begin(),parts_.end(),[](const auto& p){return p.kind==PartKind::PiezoBuzzer||p.kind==PartKind::Speaker;}))return signals;
    for(const auto& [id,board]:boards)if(board)for(int gpio:board->profile().gpio){
        const auto& pin=board->pin(gpio);if(!pin.frequency||pin.pwm||pin.mode!=PinMode::Output)continue;
        auto probe=*board;auto sources=boards;sources[id]=&probe;
        probe.digitalWrite(gpio,true);const auto high=solve(sources);
        probe.digitalWrite(gpio,false);const auto low=solve(sources);
        if(!high.errors.empty()||!low.errors.empty()){
            if(errors){errors->insert(errors->end(),high.errors.begin(),high.errors.end());errors->insert(errors->end(),low.errors.begin(),low.errors.end());}
            continue;
        }
        for(const auto& part:parts_)if(part.kind==PartKind::PiezoBuzzer||part.kind==PartKind::Speaker){
            auto difference=[&](const CircuitState& state)->std::optional<double>{
                const auto& a=state.voltage.at({part.id,"+"});const auto& b=state.voltage.at({part.id,"-"});
                if(!a||!b)return {};
                return *a-*b;
            };
            const auto a=difference(high),b=difference(low);if(!a||!b)continue;
            // Normalize each transducer to a useful listening level; retain resistor attenuation.
            const double reference=part.kind==PartKind::Speaker?0.25:board->profile().voltage;
            const double amplitude=(*a-*b)/reference;
            if(std::abs(amplitude)>1e-6)signals.push_back({part.id+":"+id+":"+std::to_string(gpio),double(pin.frequency),std::clamp(amplitude,-1.0,1.0)});
        }
    }
    return signals;
}
bool CircuitDocument::oledReady(const std::string& id,int sda,int scl,const CircuitState& state,const std::string& board) const {
    if(part(id).kind!=PartKind::Oled||sda==scl)return false;
    const auto v=state.voltage.find({id,"VCC"}),g=state.voltage.find({id,"GND"});
    if(v==state.voltage.end()||g==state.voltage.end()||!v->second||!g->second||*v->second-*g->second<3.0||*v->second-*g->second>3.6)return false;
    if(connected({id,"SDA"},{id,"SCL"}))return false;
    if(state.level({id,"SDA"})!=Level::High||state.level({id,"SCL"})!=Level::High)return false;
    const auto active=board.empty()?boardId():board;if(active.empty())return false;
    const auto profile=BoardProfile::get(part(active).model);
    return connected({id,"GND"},{active,"GND"})&&connected({id,"SDA"},{active,profile.pinName(sda)})&&connected({id,"SCL"},{active,profile.pinName(scl)});
}
}
