#include "thinkercad/electronics.hpp"
#include "thinkercad/actuators.hpp"
#include <algorithm>
#include <cmath>
namespace thinkercad {
void registerElectronics(std::map<std::string,ModuleDefinition>& catalog){
    registerActuators(catalog);
    auto add=[&](std::string id,std::string label,std::vector<std::string> pins,std::vector<ModuleParameter> parameters={},double width=7.62,double height=20,bool powered=true){ModuleDefinition d;d.id=id;d.label=label;d.bus="electrical";d.implementation=id;d.pins=pins;d.parameters=parameters;d.width=width;d.height=height;d.builtin=true;d.minVoltage=2;d.maxVoltage=6;d.idleCurrent=powered?.00001:0;if(!powered)d.power=d.ground="";for(std::size_t i=0;i<pins.size();++i){const int half=int((pins.size()+1)/2);if(1.5+(half-1)*2.54>height)d.pinPositions[pins[i]]={1.27+i*2.54,height};else d.pinPositions[pins[i]]={i<std::size_t(half)?0:width,1.5+(i<std::size_t(half)?i:pins.size()-1-i)*2.54};}catalog.emplace(id,d);};
    add("cd74hc4067","CD74HC4067 · 16-channel analog multiplexer",{"C9","C10","C11","C12","C13","C14","C15","SIG","EN","S0","S1","GND","S3","S2","C0","C1","C2","C3","C4","C5","C6","C7","C8","VCC"},{},7.62,30);
    add("74hc165","74HC165 · parallel-in shift register",{"PL","CP","D4","D5","D6","D7","Q7N","GND","Q7","DS","D0","D1","D2","D3","CE","VCC"});
    add("74hc595","74HC595 · serial-in latched shift register",{"QB","QC","QD","QE","QF","QG","QH","GND","QHN","SRCLR","SRCLK","RCLK","OE","SER","QA","VCC"});
    add("74hc283","74HC283 · 4-bit binary adder",{"S2","B2","A2","S1","A1","B1","C0","GND","C4","S4","B4","A4","S3","A3","B3","VCC"});
    add("74hc4017","74HC4017 · decade counter",{"Q5","Q1","Q0","Q2","Q6","Q7","Q3","GND","Q8","Q4","Q9","CO","CE","CP","RESET","VCC"});
    add("74hc93","74HC93 · ripple binary counter",{"CP1","R0A","R0B","NC4","VCC","NC6","NC7","QC","QB","GND","QD","QA","NC13","CP0"});
    add("74hc4511","74HC4511 · BCD to seven segment decoder",{"B","C","LT","BI","LE","D","A","GND","e","d","c","b","a","g","f","VCC"});
    add("74hc74","74HC74 · dual D flip-flop",{"CLR1","D1","CP1","PRE1","Q1","QN1","GND","QN2","Q2","PRE2","CP2","D2","CLR2","VCC"});
    add("74hc109","74HC109 · dual J-K flip-flop",{"CLR1","J1","KN1","CP1","PRE1","Q1","QN1","GND","QN2","Q2","PRE2","CP2","KN2","J2","CLR2","VCC"});
    add("74hc373","74HC373 · octal transparent latch",{"OE","Q0","D0","D1","Q1","Q2","D2","D3","Q3","GND","LE","Q4","D4","D5","Q5","Q6","D6","D7","Q7","VCC"},{},7.62,25.9);
    add("74hc14","74HC14 · hex inverting Schmitt trigger",{"A1","Y1","A2","Y2","A3","Y3","GND","Y4","A4","Y5","A5","Y6","A6","VCC"});
    add("ne555","NE555 · timer",{"GND","TRIG","OUT","RESET","CTRL","THRESH","DISCH","VCC"},{},7.62,10.7);
    add("ne556","NE556 · dual timer",{"DISCH1","THRESH1","CTRL1","RESET1","OUT1","TRIG1","GND","TRIG2","OUT2","RESET2","CTRL2","THRESH2","DISCH2","VCC"});
    add("lm7805","LM7805 · 5 V regulator",{"IN","GND","OUT"},{},10.16,15,false);catalog.at("lm7805").maxVoltage=35;
    add("ld1117v33","LD1117V33 · 3.3 V regulator",{"GND","OUT","IN"},{},6.5,7,false);catalog.at("ld1117v33").maxVoltage=15;
    add("lm741","LM741 · operational amplifier",{"OFFSET1","IN-","IN+","V-","OFFSET5","OUT","V+","NC"},{},7.62,10.7,false);catalog.at("lm741").implementation="audio-opamp";
    add("lm393","LM393 · dual open-collector comparator",{"OUT1","IN-1","IN+1","GND","IN+2","IN-2","OUT2","VCC"},{},7.62,10.7);catalog.at("lm393").minVoltage=2;catalog.at("lm393").maxVoltage=36;
    add("lm339","LM339 · quad open-collector comparator",{"OUT2","OUT1","VCC","IN-1","IN+1","IN-2","IN+2","GND","IN+3","IN-3","IN+4","IN-4","OUT4","OUT3"});catalog.at("lm339").maxVoltage=36;
    add("4n35","4N35 · optoisolator",{"A","K","NC","E","C","B"},{},7.62,8.6,false);
    add("rotary-1p5t","Rotary switch · 1 pole / 5 positions",{"COM","1","2","3","4","5"},{{"position","Position","",1,1,5}},17,17,false);
    for(int count:{2,4,6}){std::vector<std::string> pins;for(int i=1;i<=count;++i)pins.push_back("A"+std::to_string(i));for(int i=count;i>=1;--i)pins.push_back("B"+std::to_string(i));add("dip-"+std::to_string(count),"DIP switch · "+std::to_string(count)+" × SPST",pins,{{"switches","Switch bitmask","",0,0,double((1<<count)-1)}},7.62,(count-1)*2.54+3,false);}
    add("encoder-600","Optical incremental encoder · 600 P/R",{"VCC","GND","A","B"},{{"rpm","Rotation speed","RPM",0,-3000,3000},{"angle","Shaft position","°",0,0,360}},38,38);catalog.at("encoder-600").minVoltage=5;catalog.at("encoder-600").maxVoltage=24;
    add("pir","PIR motion sensor",{"VCC","OUT","GND"},{{"motion","Motion detected","",0,0,1}},32.5,24.5);catalog.at("pir").minVoltage=4.5;catalog.at("pir").maxVoltage=20;
    for(const auto& spec:std::vector<std::pair<std::string,std::string>>{{"mq-gas","MQ gas sensor (normalized response)"},{"soil-moisture","Soil moisture sensor"},{"ambient-light","Ambient light sensor"}})add(spec.first,spec.second,{"VCC","GND","AO","DO"},{{"level","Sensor level","%",50,0,100},{"threshold","Digital threshold","%",50,0,100}},20,30);
    for(const auto& spec:std::vector<std::pair<std::string,std::string>>{{"flex","Flex sensor"},{"fsr","Force-sensitive resistor"}})add(spec.first,spec.second,{"1","2"},{{"level",spec.first=="flex"?"Bend":"Force","%",0,0,100}},spec.first=="flex"?6:18,spec.first=="flex"?95:18,false);
    add("photodiode","Photodiode",{"A","K"},{{"light","Illuminance","lux",100,0,100000}},5,5,false);
    add("zener","Zener diode",{"A","K"},{{"breakdown","Zener voltage","V",5.1,2.4,75}},12.7,3,false);
    add("solar","Solar panel",{"+","-"},{{"irradiance","Sunlight","%",100,0,100},{"voltage","Open-circuit voltage","V",6,0,24},{"current","Short-circuit current","A",.1,.001,5}},60,45,false);
    for(const auto& fruit:{"lemon","potato"})add(fruit,std::string(fruit==std::string("lemon")?"Lemon":"Potato")+" battery",{"+","-"},{{"voltage","Open-circuit voltage","V",.9,0,1.2},{"resistance","Internal resistance","ohm",1000,100,100000}},65,45,false);
    add("bulb","Automotive incandescent bulb",{"+","-"},{{"voltage","Rated voltage","V",12,1,24},{"power","Rated power","W",5,.1,60}},26,46,false);
    for(int poles:{1,2}){std::vector<std::string> pins{"COIL+","COIL-"};for(int i=1;i<=poles;++i)for(const auto* name:{"COM","NC","NO"})pins.push_back(std::string(name)+std::to_string(i));add(poles==1?"relay-spdt":"relay-dpdt",poles==1?"Relay SPDT":"Relay DPDT",pins,{{"coilVoltage","Coil pickup voltage","V",5,3,24},{"coilResistance","Coil resistance","ohm",70,20,2000}},19,15,false);}
    for(const auto& spec:std::vector<std::pair<std::string,int>>{{"jack-trrs",4},{"jack-cable-trrs",8},{"header-2",2},{"header-3",3},{"header-4",4},{"header-6",6},{"header-8",8},{"header-10",10},{"header-16",16},{"header-20",20},{"terminal-2",2},{"terminal-3",3},{"terminal-4",4},{"terminal-6",6},{"jst-2",2},{"jst-3",3},{"jst-4",4},{"rca",2},{"banana",2},{"usb-c-power",4},{"dc-barrel",3}}){std::vector<std::string> pins;for(int i=1;i<=spec.second;++i)pins.push_back("P"+std::to_string(i));if(spec.first=="jack-trrs")pins={"TIP","RING1","RING2","SLEEVE"};if(spec.first=="jack-cable-trrs")pins={"A_TIP","A_RING1","A_RING2","A_SLEEVE","B_TIP","B_RING1","B_RING2","B_SLEEVE"};add(spec.first,spec.first+" · connector",pins,{},std::max(5.,spec.second*2.54),8,false);}
    {std::vector<std::string> pins;for(int i=1;i<=10;++i)pins.push_back("A"+std::to_string(i));for(int i=10;i>=1;--i)pins.push_back("K"+std::to_string(i));add("led-bar10","10-segment LED bar graph",pins,{},7.62,25.4,false);}
    {std::vector<std::string> pins;for(int i=1;i<=8;++i)pins.push_back("R"+std::to_string(i));for(int i=1;i<=8;++i)pins.push_back("C"+std::to_string(i));add("led-matrix","LED matrix · row-anode / column-cathode",pins,{{"rows","Rows","count",8,2,16},{"columns","Columns","count",8,2,16}},32,32,false);}
    add("clock-display","4-digit seven-segment clock display",{"a","b","c","d","e","f","g","dp","COM1","COM2","COM3","COM4"},{},50,19,false);
    for(const auto* size:{"aa","aaa"})for(int cells=1;cells<=4;++cells){auto id=std::string("battery-")+size+"-"+std::to_string(cells);add(id,std::string(size==std::string("aa")?"AA":"AAA")+" battery pack · "+std::to_string(cells)+" cells",{"+","-"},{{"cells","Cells","count",double(cells),1,4}},(size==std::string("aa")?14.5:10.5)*cells,size==std::string("aa")?50.5:44.5,false);}
    add("r2r-dac","R-2R ladder · passive 8-bit DAC",{"D0","D1","D2","D3","D4","D5","D6","D7","OUT","GND"},{{"resistance","R resistance","ohm",10000,100,1000000}},25.4,12,false);
    add("active-buzzer","Active piezo buzzer",{"+","-"},{{"frequency","Oscillator frequency","Hz",2400,100,10000}},12,12,false);

    {std::vector<std::string> pins;for(int i=0;i<=20;++i)if(i!=12&&i!=17&&i!=18)pins.push_back("P"+std::to_string(i));pins.insert(pins.end(),{"3V3","GND"});add("microbit-breakout","micro:bit edge breakout",pins,{},60,25,false);}

    for(const auto& id:{"6n138","6n137"}){add(id,std::string(id)+" · MIDI optoisolator",{"NC1","A","K","NC4","GND","OUT",std::string(id)=="6n138"?"BASE":"EN","VCC"},{},7.62,10.7);catalog.at(id).minVoltage=4.5;catalog.at(id).maxVoltage=5.5;}
    for(const auto& id:{"lm13700","as13700"}){add(id,std::string(id)+" · dual OTA",{"IABC1","DIODE1","IN+1","IN-1","OUT1","V-","BUF_IN1","BUF_OUT1","BUF_OUT2","BUF_IN2","V+","OUT2","IN-2","IN+2","DIODE2","IABC2"},{},7.62,20,false);catalog.at(id).implementation="ota";}
    add("lm386","LM386 · low-voltage audio amplifier",{"GAIN1","IN-","IN+","GND","OUT","VCC","BYPASS","GAIN8"},{{"gain","Voltage gain","V/V",20,20,200}},7.62,10.7);catalog.at("lm386").minVoltage=4;catalog.at("lm386").maxVoltage=12;
    for(const auto& id:{"pam8403","tpa3116d2"}){add(id,std::string(id)+" · stereo BTL audio amplifier",{"VCC","GND","LIN","RIN","L+","L-","R+","R-"},{{"gain","Voltage gain","V/V",15.85,1,40}},std::string(id)=="pam8403"?21:52,std::string(id)=="pam8403"?19:45);catalog.at(id).minVoltage=std::string(id)=="pam8403"?2.5:4.5;catalog.at(id).maxVoltage=std::string(id)=="pam8403"?5.5:26;}
    add("midi-din-in","5-pin DIN · MIDI IN",{"P1","P2","P3","P4","P5"},{},16,16,false);
    add("midi-din-out","5-pin DIN · MIDI OUT",{"P1","P2","P3","P4","P5"},{},16,16,false);
    add("midi-din-thru","5-pin DIN · MIDI THRU",{"P1","P2","P3","P4","P5"},{},16,16,false);
    add("jack-635-ts","6.35 mm TS jack",{"TIP","SLEEVE"},{},16,28,false);
    add("jack-635-trs","6.35 mm TRS jack",{"TIP","RING","SLEEVE"},{},16,28,false);
    add("toggle-spdt","Toggle switch · SPDT ON-OFF-ON",{"A","COM","B"},{{"position","Position (0=off, 1=A, 2=B)","count",0,0,2}},12,10,false);
    add("toggle-dpdt","Toggle switch · DPDT ON-OFF-ON",{"A1","COM1","B1","A2","COM2","B2"},{{"position","Position (0=off, 1=A, 2=B)","count",0,0,2}},12,10,false);
    add("softpot","SoftPot · membrane ribbon",{"1","W","2"},{{"position","Touch position","%",50,0,100},{"touch","Touched","count",0,0,1}},12,100,false);
    add("joystick","Analog XY joystick + pushbutton",{"VCC","GND","X","Y","SW"},{{"x","X axis","%",50,0,100},{"y","Y axis","%",50,0,100},{"pressed","Pushbutton","count",0,0,1}},26,34);
    add("drum-pad","Velocity-sensitive piezo drum pad",{"+","-"},{{"strike","Strike velocity","%",0,0,100}},35,35,false);
    add("arcade-button","Arcade pushbutton",{"COM","NO"},{{"pressed","Pressed","count",0,0,1}},30,30,false);

    for(const auto* id:{"as3340","cem3340"}){add(id,std::string(id)+" · calibrated VCO",{"SCALE1","SCALE2","VEE","PULSE","PWM","HARD_SYNC","HF_TRACK","SAW","SOFT_SYNC","TRI","CAP","GND","LINEAR_FM","SCALE","CV","VCC"},{{"frequency","Frequency at 0 V CV","Hz",440,.1,20000}},7.62,20);catalog.at(id).minVoltage=8;catalog.at(id).maxVoltage=18;}
    add("xr2206","XR2206 · function generator",{"AM","SINE_TRI","MULT","VCC","CAP1","CAP2","R1","R2","FSK","BIAS","SQUARE","GND","SHAPE1","SHAPE2","SYMM1","SYMM2"},{{"frequency","Calibrated frequency","Hz",440,.1,20000},{"waveform","Output (1=sine, 2=triangle)","count",1,1,2}},7.62,20);catalog.at("xr2206").minVoltage=10;catalog.at("xr2206").maxVoltage=26;
    add("icl8038","ICL8038 · function generator",{"SHAPE1","SINE","TRI","DUTY1","DUTY2","VCC","FM_BIAS","FM","SQUARE","CAP","GND","SHAPE2","NC13","NC14"},{{"frequency","Calibrated frequency","Hz",440,.1,20000}},7.62,20);catalog.at("icl8038").minVoltage=10;catalog.at("icl8038").maxVoltage=30;
    add("noise-generator","Noise generator · white / pink / brown",{"VCC","GND","OUT"},{{"color","Noise (0=white, 1=pink, 2=brown)","count",0,0,2},{"amplitude","Peak amplitude","V",1,.01,5}},18,25);

    for(const auto& id:{"ay-3-8910","ym2149f"}){add(id,std::string(id)+" · three-channel PSG",{"GND","NC2","OUTB","OUTA","NC5","IOB7","IOB6","IOB5","IOB4","IOB3","IOB2","IOB1","IOB0","IOA7","IOA6","IOA5","IOA4","IOA3","IOA2","IOA1","IOA0","CLK","RESET","A9","A8","TEST2","BDIR","BC2","BC1","D7","D6","D5","D4","D3","D2","D1","D0","OUTC","TEST1","VCC"},{{"clock","Master clock","Hz",2000000,100000,4000000}},15.24,53);catalog.at(id).minVoltage=4.5;catalog.at(id).maxVoltage=5.5;}
    add("sn76489","SN76489 · tone/noise PSG",{"D5","D6","D7","READY","WE","CE","AUDIO","GND","NC9","D0","D1","D2","D3","CLK","D4","VCC"},{{"clock","Master clock","Hz",3579545,100000,4000000}},7.62,20);catalog.at("sn76489").minVoltage=4.5;catalog.at("sn76489").maxVoltage=5.5;

    add("cd4051b","CD4051B · bidirectional 8-channel analog multiplexer",{"C4","C6","SIG","C7","C5","INH","VEE","GND","S2","S1","S0","C3","C0","C1","C2","VCC"},{},7.62,20);catalog.at("cd4051b").minVoltage=3;catalog.at("cd4051b").maxVoltage=20;
    add("cd4052b","CD4052B · dual bidirectional 4-channel analog multiplexer",{"Y0","Y2","Y","Y3","Y1","INH","VEE","GND","S1","S0","X3","X","X0","X1","X2","VCC"},{},7.62,20);catalog.at("cd4052b").minVoltage=3;catalog.at("cd4052b").maxVoltage=20;
    add("matrix-patch","4×4 patch-pin crossbar matrix",{"R1","R2","R3","R4","C1","C2","C3","C4"},{{"connections","Patch bitmask (row-major)","count",0,0,65535}},40,40,false);
    add("toggle-spdt-on-on","Toggle switch · SPDT ON-ON",{"A","COM","B"},{{"position","Position (1=A, 2=B)","count",1,1,2}},12,10,false);
    add("toggle-dpdt-on-on","Toggle switch · DPDT ON-ON",{"A1","COM1","B1","A2","COM2","B2"},{{"position","Position (1=A, 2=B)","count",1,1,2}},12,10,false);

    add("ssi2144","SSI2144 · calibrated 4-pole low-pass filter module",{"VCC","VEE","GND","IN","CV","RES","OUT"},{{"cutoff","Cutoff at 0 V CV","Hz",1000,20,20000},{"resonance","Resonance","",0,0,.95}},25,30);catalog.at("ssi2144").minVoltage=8;catalog.at("ssi2144").maxVoltage=18;
    add("pt2399","PT2399 · warm echo/delay module",{"VCC","GND","IN","OUT"},{{"delay","Delay","ms",150,30,340},{"feedback","Feedback","",.35,0,.85},{"mix","Wet mix","",.5,0,1}},28,32);catalog.at("pt2399").minVoltage=4.5;catalog.at("pt2399").maxVoltage=5.5;
    add("fv1","FV-1 · behavioral stereo effects module",{"VCC","GND","INL","INR","OUTL","OUTR","POT0","POT1","POT2","P0","P1","P2"},{{"program","Effect program","count",0,0,7},{"mix","Wet mix","",.5,0,1},{"delay","Delay","ms",250,1,1500},{"feedback","Feedback","",.5,0,.85},{"pitch","Pitch shift","semitones",0,-12,12}},40,35);catalog.at("fv1").minVoltage=3;catalog.at("fv1").maxVoltage=3.6;

    for(const auto& id:{"tl072","ne5532","opa2134"}){add(id,std::string(id)+" · dual audio op-amp",{"OUT1","IN-1","IN+1","V-","IN+2","IN-2","OUT2","V+"},{},7.62,10.7,false);catalog.at(id).implementation="audio-opamp";}
    add("tl074","TL074 · quad audio op-amp",{"OUT1","IN-1","IN+1","V+","IN+2","IN-2","OUT2","OUT3","IN-3","IN+3","V-","IN+4","IN-4","OUT4"},{},7.62,20,false);catalog.at("tl074").implementation="audio-opamp";
    add("lm317","LM317 · adjustable positive regulator",{"ADJ","OUT","IN"},{},10.16,15,false);
    add("lm337","LM337 · adjustable negative regulator",{"ADJ","IN","OUT"},{},10.16,15,false);
    for(const auto* name:{"tc7660","max1044","icl7660"}){add(name,std::string(name)+" · voltage inverter",{"BOOST","CAP+","GND","CAP-","VOUT","LV","OSC","VCC"},{},7.62,10.7);catalog.at(name).minVoltage=1.5;catalog.at(name).maxVoltage=10;}
    for(const auto* id:{"lm7805","ld1117v33","lm317","lm337"}){auto& regulator=catalog.at(id);regulator.width=10.16;regulator.height=25.4;for(std::size_t pin=0;pin<regulator.pins.size();++pin)regulator.pinPositions[regulator.pins[pin]]={2.54+pin*2.54,25.4};}
    add("tl431","TL431 · 2.495 V shunt reference",{"REF","ANODE","CATHODE"},{},4.6,4.6,false);
    auto rowPins=[&](const std::string& id,double width,double height,double pitch){auto& model=catalog.at(id);model.width=width;model.height=height;model.pinPositions.clear();for(std::size_t i=0;i<model.pins.size();++i)model.pinPositions[model.pins[i]]={width/2+(double(i)-(model.pins.size()-1)/2.)*pitch,height};};
    rowPins("joystick",26,34,2.54);rowPins("softpot",12,100,2.54);rowPins("fsr",18,55,2.54);rowPins("photodiode",5,12.7,2.54);rowPins("tl431",4.6,12.7,1.27);
    for(int count:{2,3,4,6,8,10,16,20})rowPins("header-"+std::to_string(count),count*2.54,2.54,2.54);
    for(int count:{2,3,4,6})rowPins("terminal-"+std::to_string(count),count*3.5,8,3.5);
    for(int count:{2,3,4})rowPins("jst-"+std::to_string(count),5+(count-1)*2,8,2);
    rowPins("rca",12,12,5.08);rowPins("banana",16,10,8);rowPins("dc-barrel",10,14,2.54);rowPins("usb-c-power",15.24,15.24,2.54);
    catalog.at("banana").label="Banana socket pair";
    for(int count:{2,3,4,6,8,10,16,20})for(auto& [name,at]:catalog.at("header-"+std::to_string(count)).pinPositions)at.second=1.27;
    std::swap(catalog.at("joystick").pinPositions.at("GND"),catalog.at("joystick").pinPositions.at("VCC"));


}
std::vector<ElectricalBranch> electronicBranches(const CircuitPart& p,const std::function<std::optional<double>(const std::string&)>& voltage,const DeviceRegisters* memory,double time,const std::map<std::string,AnalogDeviceState>* analog){
    std::vector<ElectricalBranch> result;
    if(actuatorBranches(p,voltage,memory,result))return result;
    const auto& model=ModuleRegistry::get(p.model);auto add=[&](const std::string& a,const std::string& b,double r,double offset=0){result.push_back({a,b,std::max(.001,r),offset});};
    auto v=[&](const std::string& name,double fallback=0){return voltage(name).value_or(fallback);};const auto id=p.model;
    if(id.starts_with("battery-")){add("+","-",.15*parameter(p,"cells",double(id.back()-'0')),1.5*parameter(p,"cells",double(id.back()-'0')));return result;}
    if(id=="r2r-dac"){
        // Eliminate seven internal ladder junctions, preserving the complete
        // passive multiport network, including GPIO loading and output impedance.
        static const auto reduced=[] {
            std::array<std::array<double,17>,17> g{};
            auto resistor=[&](int a,int b,double conductance){g[a][a]+=conductance;g[b][b]+=conductance;g[a][b]-=conductance;g[b][a]-=conductance;};
            for(int bit=0;bit<8;++bit){int junction=bit==7?8:10+bit;resistor(bit,junction,.5);if(bit<7)resistor(junction,bit==6?8:11+bit,1);}
            resistor(10,9,.5);
            for(int k=10;k<17;++k){for(int a=0;a<17;++a)if(a!=k)for(int b=0;b<17;++b)if(b!=k)g[a][b]-=g[a][k]*g[k][b]/g[k][k];for(int a=0;a<17;++a)g[a][k]=g[k][a]=0;}
            return g;
        }();
        const auto& pins=model.pins;double resistance=parameter(p,"resistance",10000);
        for(int a=0;a<10;++a)for(int b=a+1;b<10;++b)if(reduced[a][b]<-1e-14)add(pins[a],pins[b],resistance/-reduced[a][b]);
        return result;
    }
    if(id=="active-buzzer"){add("+","-",1000);return result;}
    if(id=="flex"||id=="fsr"){double level=parameter(p,"level",0)/100;add("1","2",id=="flex"?25000+75000*level:1000000*std::pow(.0001,level));return result;}
    if(id=="rotary-1p5t"){add("COM",std::to_string(std::clamp(int(parameter(p,"position",1)),1,5)),.01);return result;}
    if(id.starts_with("dip-")){int count=std::stoi(id.substr(4)),mask=int(parameter(p,"switches",0));for(int i=1;i<=count;++i)if(mask&(1<<(i-1)))add("A"+std::to_string(i),"B"+std::to_string(i),.01);return result;}
    if(id=="jack-cable-trrs"){for(const auto* pin:{"TIP","RING1","RING2","SLEEVE"})add(std::string("A_")+pin,std::string("B_")+pin,.1);return result;}
    if(id=="solar"){double voc=parameter(p,"voltage",6)*std::sqrt(parameter(p,"irradiance",100)/100),isc=parameter(p,"current",.1)*parameter(p,"irradiance",100)/100;add("+","-",std::max(1.,voc/std::max(.000001,isc)),voc);return result;}
    if(id=="lemon"||id=="potato"){add("+","-",parameter(p,"resistance",1000),parameter(p,"voltage",.9));return result;}
    if(id=="bulb"){double rated=parameter(p,"voltage",12);add("+","-",rated*rated/parameter(p,"power",5));return result;}
    if(id=="photodiode"){const double photocurrent=parameter(p,"light",100)*.000000005;add("A","K",1e8,-photocurrent*1e8);if(v("A")-v("K")>.6)add("A","K",20,.6);return result;}
    if(id=="zener"){if(v("K")-v("A")>parameter(p,"breakdown",5.1))add("K","A",5,parameter(p,"breakdown",5.1));else if(v("A")-v("K")>.7)add("A","K",5,.7);else add("A","K",1e10);return result;}
    if(id=="4n35"){add("A","K",v("A")-v("K")>1.2?30:1e10,v("A")-v("K")>1.2?1.2:0);double current=std::max(0.,(v("A")-v("K")-1.2)/30);add("C","E",current>0?std::max(5.,std::max(0.,v("C")-v("E"))/(current*.5)):1e10);add("B","E",1e8);return result;}
    if(id.starts_with("relay-")){double resistance=parameter(p,"coilResistance",70);add("COIL+","COIL-",resistance);bool on=std::abs(v("COIL+")-v("COIL-"))>=parameter(p,"coilVoltage",5)*.75;for(int i=1;i<=(id=="relay-dpdt"?2:1);++i)add("COM"+std::to_string(i),std::string(on?"NO":"NC")+std::to_string(i),.05);return result;}
    if(id=="lm7805"||id=="ld1117v33"){double vin=v("IN")-v("GND"),dropout=id=="lm7805"?2:1.1,target=id=="lm7805"?5:3.3;if(vin>0&&vin<=model.maxVoltage)add("OUT","GND",.1,std::clamp(vin-dropout,0.,target));add("IN","GND",10000);return result;}
    if(id=="lm741")return result;
    if(id=="lm317"||id=="lm337"){const double sign=id=="lm317"?1:-1;double headroom=sign*(v("IN")-v("ADJ"));if(headroom>0&&headroom<40)add("OUT","ADJ",.1,sign*std::clamp(headroom-2,0.,1.25));return result;}
    if(id=="tl431"){add("REF","ANODE",1e8);if(v("REF")-v("ANODE")>2.495)add("CATHODE","ANODE",10);return result;}
    if(id=="matrix-patch"){int mask=int(parameter(p,"connections",0));for(int row=0;row<4;++row)for(int col=0;col<4;++col)if(mask&(1<<(row*4+col)))add("R"+std::to_string(row+1),"C"+std::to_string(col+1),.02);return result;}
    if(id.starts_with("toggle-")){int position=int(parameter(p,"position",id.ends_with("on-on")?1:0));if(position)for(int i=1;i<=(id.find("dpdt")!=std::string::npos?2:1);++i){auto n=id.find("dpdt")!=std::string::npos?std::to_string(i):"";add("COM"+n,std::string(position==1?"A":"B")+n,.02);}return result;}
    if(id=="softpot"){if(parameter(p,"touch",0)){double pos=parameter(p,"position",50)/100;add("1","W",std::max(.01,10000*pos));add("W","2",std::max(.01,10000*(1-pos)));}return result;}
    if(id=="arcade-button"){if(parameter(p,"pressed",0))add("COM","NO",.01);return result;}
    if(id=="drum-pad"){double signal=analog&&analog->contains(p.id)?analog->at(p.id).outL:0;add("+","-",10000,signal);return result;}
    if(id=="6n138"||id=="6n137"){double forward=v("A")-v("K");add("A","K",forward>1.3?30:1e10,forward>1.3?1.3:0);if(v("VCC")-v("GND")>=4.5&&v("VCC")-v("GND")<=5.5&&forward>1.33&&(id=="6n138"||v("EN")-v("GND")>=2))add("OUT","GND",25);return result;}
    if(id=="mh-z19b"){
        if(voltage("VCC")&&voltage("GND")&&v("VCC")-v("GND")>=4.5&&v("VCC")-v("GND")<=5.5)add("TX","GND",50,3.3);
        add("RX","GND",1000000);return result;
    }
    if(model.power.empty())return result;
    const auto supply=voltage(model.power),ground=voltage(model.ground);if(!supply||!ground||*supply-*ground<model.minVoltage||*supply-*ground>model.maxVoltage)return result;
    const double lo=*ground,hi=*supply,range=hi-lo;auto high=[&](const std::string& pin){return voltage(pin)&&v(pin)-lo>=range*.5;};
    auto out=[&](const std::string& pin,bool value){add(pin,model.ground,35,value?range:0);};std::array<std::uint64_t,16> state{};if(memory&&memory->contains(p.id))state=memory->at(p.id);
    if(id=="ssi2144"||id=="pt2399"||id=="fv1"){if(analog&&analog->contains(p.id)){const auto& effect=analog->at(p.id);add(id=="fv1"?"OUTL":"OUT","GND",100,effect.outL);if(id=="fv1")add("OUTR","GND",100,effect.outR);}return result;}
    if(id=="ay-3-8910"||id=="ym2149f"){
        auto reg=[&](int index){return int((state[index/8]>>((index%8)*8))&255);};double clock=parameter(p,"clock",2000000);int mixer=reg(7);bool noise=state[7]&1;for(int channel=0;channel<3;++channel){int period=reg(channel*2)|((reg(channel*2+1)&15)<<8);double frequency=clock/(16*std::max(1,period));bool tone=std::fmod(time*frequency,1.)<.5;int volume=reg(8+channel)&15;if(reg(8+channel)&16){int periodEnv=std::max(1,reg(11)|(reg(12)<<8));auto steps=std::uint64_t(std::max(0.,time-double(state[5])*1e-6)*clock/(256*periodEnv));int shape=reg(13)&15,position=int(steps%16);if(steps>=16&&!(shape&8))volume=0;else if(steps>=16&&(shape&1))volume=((bool(shape&4))!=bool(shape&2))?15:0;else {if((shape&2)&&((steps/16)%2))position=15-position;volume=(shape&4)?position:15-position;}}
            double amplitude=volume?std::pow(10.,(volume-15)*1.5/20):0;add(std::string("OUT")+char('A'+channel),"GND",1000,((mixer&(1<<channel))||tone)&&((mixer&(8<<channel))||noise)?amplitude:0);
        }
        int mode=(high("BDIR")?4:0)|(high("BC2")?2:0)|(high("BC1")?1:0);if(mode==3){int value=reg(state[2]&15);for(int i=0;i<8;++i)out("D"+std::to_string(i),value&(1<<i));}for(int port=0;port<2;++port)if(mixer&(64<<port)){int value=reg(14+port);for(int i=0;i<8;++i)out(std::string("IO")+char('A'+port)+std::to_string(i),value&(1<<i));}return result;
    }
    if(id=="sn76489"){double clock=parameter(p,"clock",3579545),value=0;for(int channel=0;channel<3;++channel){int attenuation=(state[4]>>(channel*4))&15;if(attenuation<15){double frequency=clock/(32*std::max<std::uint64_t>(1,state[channel]));value+=(std::fmod(time*frequency,1.)<.5?1:-1)*std::pow(10.,-attenuation/10.)*.2;}}int attenuation=(state[4]>>12)&15;if(attenuation<15){value+=(state[8]&1?1:-1)*std::pow(10.,-attenuation/10.)*.2;}add("AUDIO","GND",1000,.8+value);out("READY",true);return result;}
    if(id=="as3340"||id=="cem3340"||id=="xr2206"||id=="icl8038"){
        double frequency=parameter(p,"frequency",440);if(id=="as3340"||id=="cem3340"){auto negative=voltage("VEE");if(!negative||*negative-lo> -4||*negative-lo< -18)return result;frequency=std::clamp(frequency*std::pow(2.,std::clamp(v("CV")-lo,-12.,12.)),.01,20000.);}
        double phase=(time-state[14]*1e-6)*frequency-std::floor((time-state[14]*1e-6)*frequency),triangle=1-4*std::abs(phase-.5),sine=std::sin(6.283185307179586*phase);
        if(id=="as3340"||id=="cem3340"){add("SAW","GND",100,range*.8*phase);add("TRI","GND",100,range*.25*(triangle+1));double duty=std::clamp((v("PWM",lo+2.5)-lo)/5,.02,.98);add("PULSE","GND",100,phase<duty?range:0);}
        else if(id=="xr2206"){add("SINE_TRI","GND",600,range/2+range*.15*(parameter(p,"waveform",1)==1?sine:triangle));if(phase<.5)add("SQUARE","GND",25);}
        else{add("SINE","GND",200,range/2+range*.11*sine);add("TRI","GND",200,range/2+range*.165*triangle);if(phase<.5)add("SQUARE","GND",25);}return result;
    }
    if(id=="noise-generator"){std::uint64_t seed=14695981039346656037ULL;for(unsigned char byte:p.id){seed^=byte;seed*=1099511628211ULL;}auto noise=[seed](std::uint64_t index){index+=seed;std::uint64_t x=index+0x9e3779b97f4a7c15ULL;x=(x^(x>>30))*0xbf58476d1ce4e5b9ULL;x=(x^(x>>27))*0x94d049bb133111ebULL;x^=x>>31;return double(x&0xffffff)/8388607.5-1;};auto sample=std::uint64_t(std::max(0.,time)*48000);double value=noise(sample);if(parameter(p,"color",0)>=1){value=0;for(int i=0;i<12;++i)value+=noise((sample>>i)+std::uint64_t(i)*1234567)*(parameter(p,"color",0)==2?std::pow(2.,i*.5):1);value/=parameter(p,"color",0)==2?150:6;}add("OUT","GND",100,range/2+std::clamp(value,-1.,1.)*std::min(parameter(p,"amplitude",1),range/2));return result;}
    if(id=="lm386"){add("OUT","GND",1,std::clamp(range/2+parameter(p,"gain",20)*(v("IN+")-v("IN-")),.2,range-.2));return result;}
    if(id=="pam8403"||id=="tpa3116d2"){for(const auto& channel:{"L","R"}){double input=v(std::string(channel)+"IN")-lo,target=std::clamp(input*parameter(p,"gain",15.85),-range/2+.1,range/2-.1);add(std::string(channel)+"+","GND",.2,range/2+target);add(std::string(channel)+"-","GND",.2,range/2-target);}return result;}
    if(id=="joystick"){add("X","GND",1000,range*parameter(p,"x",50)/100);add("Y","GND",1000,range*parameter(p,"y",50)/100);if(parameter(p,"pressed",0))add("SW","GND",.01);return result;}
    if(id=="tc7660"||id=="max1044"||id=="icl7660"){add("VOUT","GND",20,-range);return result;}
    if(id=="cd4051b"||id=="cd4052b"){auto negative=voltage("VEE");if(!negative||hi-*negative>20||high("INH"))return result;int selected=int(high("S0"))+2*int(high("S1"));if(id=="cd4051b")add("SIG","C"+std::to_string(selected+4*int(high("S2"))),2350/range);else for(const auto* side:{"X","Y"})add(side,std::string(side)+std::to_string(selected),2350/range);return result;}
    if(id=="cd74hc4067"){if(!high("EN")){int selected=0;for(int i=0;i<4;++i)selected|=int(high("S"+std::to_string(i)))<<i;add("SIG","C"+std::to_string(selected),70);}return result;}
    if(id=="74hc283"){int a=0,b=0;for(int i=1;i<=4;++i){a|=int(high("A"+std::to_string(i)))<<(i-1);b|=int(high("B"+std::to_string(i)))<<(i-1);}int sum=a+b+int(high("C0"));for(int i=1;i<=4;++i)out("S"+std::to_string(i),sum&(1<<(i-1)));out("C4",sum&16);}
    else if(id=="74hc165"){auto value=state[0];if(!high("PL")){value=0;for(int i=0;i<8;++i)value|=std::uint64_t(high("D"+std::to_string(i)))<<i;}out("Q7",value&128);out("Q7N",!(value&128));}
    else if(id=="74hc595"){out("QHN",state[0]&128);if(!high("OE"))for(int i=0;i<8;++i)out("Q"+std::string(1,char('A'+i)),state[1]&(1<<i));}
    else if(id=="74hc373"){auto data=state[0];if(high("LE")){data=0;for(int i=0;i<8;++i)data|=std::uint64_t(high("D"+std::to_string(i)))<<i;}if(!high("OE"))for(int i=0;i<8;++i)out("Q"+std::to_string(i),data&(1<<i));}
    else if(id=="74hc4017"){auto value=high("RESET")?0:state[0]%10;for(int i=0;i<10;++i)out("Q"+std::to_string(i),value==unsigned(i));out("CO",value<5);}
    else if(id=="74hc93"){auto value=high("R0A")&&high("R0B")?0:state[0];for(int i=0;i<4;++i)out("Q"+std::string(1,char('A'+i)),value&(1<<i));}
    else if(id=="74hc74"||id=="74hc109"){for(int i=1;i<=2;++i){auto n=std::to_string(i);bool set=state[i-1];if(!high("CLR"+n))set=false;if(!high("PRE"+n))set=true;out("Q"+n,set);out("QN"+n,(!high("CLR"+n)&&!high("PRE"+n))||!set);}}
    else if(id=="74hc4511"){int data=state[0];if(!high("LE"))data=int(high("A"))+2*int(high("B"))+4*int(high("C"))+8*int(high("D"));const int digits[]={0x3f,6,0x5b,0x4f,0x66,0x6d,0x7d,7,0x7f,0x6f};int mask=!high("BI")?0:!high("LT")?127:data<10?digits[data]:0;for(int i=0;i<7;++i)out(std::string(1,char('a'+i)),mask&(1<<i));}
    else if(id=="74hc14"){for(int i=1;i<=6;++i)out("Y"+std::to_string(i),voltage("A"+std::to_string(i))?(v("A"+std::to_string(i))-lo>range*.6?false:v("A"+std::to_string(i))-lo<range*.4?true:!(state[0]&(1<<i))):!(state[0]&(1<<i)));}
    else if(id=="ne555"||id=="ne556"){for(int i=0;i<(id=="ne555"?1:2);++i){auto n=id=="ne555"?"":std::to_string(i+1);bool set=state[i];if(!high("RESET"+n))set=false;out("OUT"+n,set);if(!set)add("DISCH"+n,"GND",10);}}
    else if(id=="lm393"||id=="lm339"){for(int i=1;i<=(id=="lm393"?2:4);++i){auto n=std::to_string(i);if(voltage("IN+"+n)&&voltage("IN-"+n)&&v("IN+"+n)<v("IN-"+n))add("OUT"+n,"GND",25);}}
    else if(id=="encoder-600"){long count=long(std::floor((parameter(p,"angle",0)/360+parameter(p,"rpm",0)*time/60)*2400));int phase=int((count%4+4)%4);if(phase==0||phase==3)add("A","GND",20);if(phase==0||phase==1)add("B","GND",20);}
    else if(id=="pir")add("OUT","GND",100,parameter(p,"motion",0)?std::min(3.3,range):0);
    else if(id=="mq-gas"||id=="soil-moisture"||id=="ambient-light"){add("AO","GND",1000,range*parameter(p,"level",50)/100);out("DO",parameter(p,"level",50)>=parameter(p,"threshold",50));}
    return result;
}
void advanceAnalogDevices(const CircuitDocument& doc,const CircuitState& circuit,ReactiveState& memory,Microseconds now){
    for(const auto& p:doc.parts())if(p.kind==PartKind::Module&&p.model=="drum-pad"){
        auto& pad=memory.analog[p.id];double strike=parameter(p,"strike",0)/100;
        if(strike>pad.filter[0]){pad.filter[1]=double(now);pad.filter[2]=strike;}
        pad.filter[0]=strike;double age=std::max(0.,(double(now)-pad.filter[1])*1e-6);
        pad.outL=5*pad.filter[2]*std::exp(-age/.008)*std::cos(age*6.283185307179586*2000);
    }

    for(const auto& p:doc.parts())if(p.kind==PartKind::Module&&(p.model=="ssi2144"||p.model=="pt2399"||p.model=="fv1")){
        auto value=[&](const std::string& pin,double fallback=0){auto it=circuit.voltage.find({p.id,pin});return it!=circuit.voltage.end()&&it->second?*it->second:fallback;};const auto& model=ModuleRegistry::get(p.model);double ground=value("GND"),supply=value("VCC")-ground;auto& effect=memory.analog[p.id];if(supply<model.minVoltage||supply>model.maxVoltage){effect=AnalogDeviceState{};effect.last=now;continue;}if(now<=effect.last)continue;double dt=(now-effect.last)*1e-6;effect.last=now;
        if(p.model=="ssi2144"){if(value("VEE")-ground> -4){effect.outL=0;continue;}double cutoff=std::clamp(parameter(p,"cutoff",1000)*std::pow(2.,value("CV")-ground),20.,20000.),alpha=1-std::exp(-6.283185307179586*cutoff*dt);double resonance=parameter(p,"resonance",0);double input=std::tanh((value("IN")-ground-4*resonance*effect.filter[3])/5)*5;for(int pole=0;pole<4;++pole){effect.filter[pole]+=alpha*(input-effect.filter[pole]);input=effect.filter[pole];}effect.outL=std::clamp(effect.filter[3],value("VEE")-ground+.5,supply-.5);continue;}
        if(effect.delay.empty())effect.delay.resize(96000);double inputL=value(p.model=="fv1"?"INL":"IN")-ground,inputR=p.model=="fv1"?value("INR")-ground:inputL;effect.accumulator+=dt*48000;
        auto tap=[&](double delay,int channel){double index=double(effect.cursor)-std::clamp(delay*48,1.,95998.);while(index<0)index+=effect.delay.size();auto a=std::size_t(index)%effect.delay.size(),b=(a+1)%effect.delay.size();return effect.delay[a][channel]+(effect.delay[b][channel]-effect.delay[a][channel])*(index-std::floor(index));};
        int budget=0;while(effect.accumulator>=1&&budget++<48000){effect.accumulator-=1;double delay=parameter(p,"delay",p.model=="fv1"?250:150),feedback=parameter(p,"feedback",.35),mix=parameter(p,"mix",.5),wetL=tap(delay,0),wetR=tap(delay,1);int program=int(parameter(p,"program",0));
            if(p.model=="fv1"){
                bool wired=false;int selected=0;for(int bit=0;bit<3;++bit){auto it=circuit.voltage.find({p.id,"P"+std::to_string(bit)});if(it!=circuit.voltage.end()&&it->second){wired=true;if(*it->second-ground>supply*.5)selected|=1<<bit;}}if(wired)program=selected;
                double t=now*1e-6;
                if(program==0||program==1||program==7){double spread=program==1?1.6:program==7?.65:1;wetL=(tap(37*spread,0)+tap(53*spread,1)+tap(79*spread,0)+tap(113*spread,1))*.25;wetR=(tap(41*spread,1)+tap(61*spread,0)+tap(83*spread,1)+tap(127*spread,0))*.25;}
                else if(program==2||program==3){double base=program==2?20:2,depth=program==2?8:1.8;wetL=tap(base+depth*std::sin(t*6.283185307*.7),0);wetR=tap(base+depth*std::sin(t*6.283185307*.7+1.57),1);}
                else if(program==4){wetL=inputL*(.5+.5*std::sin(t*6.283185307*5));wetR=inputR*(.5+.5*std::sin(t*6.283185307*5));feedback=0;}
                else if(program==5){double ratio=std::pow(2.,parameter(p,"pitch",0)/12),phase=effect.filter[4];phase+=(1-ratio)/2048;phase-=std::floor(phase);effect.filter[4]=phase;auto shift=[&](int channel){double other=std::fmod(phase+.5,1.),a=.5-.5*std::cos(phase*6.283185307),b=.5-.5*std::cos(other*6.283185307);return tap((64+phase*2048)/48,channel)*a+tap((64+other*2048)/48,channel)*b;};wetL=shift(0);wetR=shift(1);feedback=0;}
            }
            double driveL=inputL+wetR*feedback,driveR=inputR+wetL*feedback;if(p.model=="pt2399"){double alpha=1-std::exp(-6.283185307179586*4500/48000);effect.filter[0]+=alpha*(std::tanh(driveL*.8)/.8-effect.filter[0]);effect.filter[1]+=alpha*(std::tanh(driveR*.8)/.8-effect.filter[1]);driveL=effect.filter[0];driveR=effect.filter[1];}
            effect.delay[effect.cursor]={driveL,driveR};effect.cursor=(effect.cursor+1)%effect.delay.size();effect.outL=std::clamp(inputL*(1-mix)+wetL*mix,-1.5,1.5);effect.outR=std::clamp(inputR*(1-mix)+wetR*mix,-1.5,1.5);
        }
    }
}
void observeElectronics(const CircuitDocument& doc,const CircuitState& circuit,DeviceRegisters& memory,Microseconds now){
    observeActuators(doc,circuit,memory,now);
    for(const auto& p:doc.parts())if(p.kind==PartKind::Module){const auto& model=ModuleRegistry::get(p.model);if(model.bus!="electrical"||model.power.empty())continue;auto read=[&](const std::string& name)->std::optional<double>{auto it=circuit.voltage.find({p.id,name});return it==circuit.voltage.end()?std::nullopt:it->second;};auto supply=read(model.power),ground=read(model.ground);if(!supply||!ground||*supply-*ground<model.minVoltage||*supply-*ground>model.maxVoltage){memory.erase(p.id);continue;}auto& s=memory[p.id];const double range=*supply-*ground;auto high=[&](const std::string& name){auto v=read(name);return v&&*v-*ground>=range*.5;};auto edge=[&](const std::string& pin,int slot,bool rising=true){bool value=high(pin),old=s[slot];s[slot]=value;return rising?value&&!old:!value&&old;};
        if(p.model=="as3340"||p.model=="cem3340"){if(edge("HARD_SYNC",13))s[14]=now;}
        else if(p.model=="ay-3-8910"||p.model=="ym2149f"){
            auto readReg=[&](int index){return int((s[index/8]>>((index%8)*8))&255);};auto writeReg=[&](int index,int value){auto shift=(index%8)*8;s[index/8]=(s[index/8]&~(std::uint64_t(255)<<shift))|(std::uint64_t(value&255)<<shift);};
            if(!high("RESET")){s={};s[7]=0x1ffff;continue;}int mode=(high("BDIR")?4:0)|(high("BC2")?2:0)|(high("BC1")?1:0),data=0;for(int i=0;i<8;++i)data|=int(high("D"+std::to_string(i)))<<i;
            if(s[3]==6&&mode!=6){writeReg(s[2]&15,s[4]);if((s[2]&15)==13)s[5]=now;}if(mode==1||mode==4||mode==7)s[2]=data&15;s[3]=mode;s[4]=data;
            auto noiseIndex=std::uint64_t(double(now)*parameter(p,"clock",2000000)/(16*std::max(1,readReg(6)&31)*1000000));if(!s[7])s[7]=0x1ffff;for(int i=0;s[6]<noiseIndex&&i<10000;++i,++s[6])s[7]=(s[7]>>1)|(((s[7]^(s[7]>>3))&1)<<16);
        }
        else if(p.model=="sn76489"){if(!s[7]){s[4]=0xffff;s[7]=1;}bool strobe=edge("WE",6,false);if(strobe&&!high("CE")){int data=0;for(int i=0;i<8;++i)data|=int(high("D"+std::to_string(i)))<<(7-i);if(data&128)s[5]=(data>>4)&7;int reg=s[5],channel=reg/2;if(reg&1)s[4]=(s[4]&~(std::uint64_t(15)<<(channel*4)))|(std::uint64_t(data&15)<<(channel*4));else if(channel==3)s[3]=data&7;else if(data&128)s[channel]=(s[channel]&0x3f0)|(data&15);else s[channel]=(s[channel]&15)|((data&63)<<4);if(reg==6){s[8]=0x4000;s[9]=0;}}
            if(!s[8])s[8]=0x4000;double divisor=(s[3]&3)==3?std::max<std::uint64_t>(1,s[2]):16<<int(s[3]&3);auto ticks=std::uint64_t(double(now)*parameter(p,"clock",3579545)/(32*divisor*1000000));for(int i=0;s[9]<ticks&&i<10000;++i,++s[9]){std::uint64_t feedback=(s[3]&4)?(s[8]^(s[8]>>1))&1:s[8]&1;s[8]=(s[8]>>1)|(feedback<<14);}
        }
        else if(p.model=="74hc595"){bool shift=edge("SRCLK",2),latch=edge("RCLK",3);if(!high("SRCLR"))s[0]=0;else if(shift)s[0]=((s[0]<<1)|high("SER"))&255;if(latch)s[1]=s[0];}
        else if(p.model=="74hc165"){bool clock=edge("CP",2);if(!high("PL")){s[0]=0;for(int i=0;i<8;++i)s[0]|=std::uint64_t(high("D"+std::to_string(i)))<<i;}else if(clock&&!high("CE"))s[0]=((s[0]<<1)|high("DS"))&255;}
        else if(p.model=="74hc373"){if(high("LE")){s[0]=0;for(int i=0;i<8;++i)s[0]|=std::uint64_t(high("D"+std::to_string(i)))<<i;}}
        else if(p.model=="74hc4017"){bool clock=edge("CP",2);if(high("RESET"))s[0]=0;else if(clock&&!high("CE"))s[0]=(s[0]+1)%10;}
        else if(p.model=="74hc93"){bool a=edge("CP0",2,false),b=edge("CP1",3,false);if(high("R0A")&&high("R0B"))s[0]=0;else {if(a)s[0]^=1;if(b)s[0]=(s[0]&1)|((s[0]+2)&14);}}
        else if(p.model=="74hc4511"){if(!high("LE"))s[0]=high("A")+2*high("B")+4*high("C")+8*high("D");}
        else if(p.model=="74hc14"){for(int i=1;i<=6;++i){auto v=read("A"+std::to_string(i));if(!v)continue;if(*v-*ground>=range*.6)s[0]|=1<<i;else if(*v-*ground<=range*.4)s[0]&=~(1<<i);}}
        else if(p.model=="74hc74"||p.model=="74hc109"){for(int i=1;i<=2;++i){auto n=std::to_string(i);bool clock=edge("CP"+n,i+1);if(!high("CLR"+n))s[i-1]=0;else if(!high("PRE"+n))s[i-1]=1;else if(clock){if(p.model=="74hc74")s[i-1]=high("D"+n);else {bool j=high("J"+n),k=!high("KN"+n);if(j&&k)s[i-1]=!s[i-1];else if(j)s[i-1]=1;else if(k)s[i-1]=0;}}}}
        else if(p.model=="ne555"||p.model=="ne556"){for(int i=0;i<(p.model=="ne555"?1:2);++i){auto n=p.model=="ne555"?"":std::to_string(i+1);const double control=read("CTRL"+n)?*read("CTRL"+n)-*ground:range*2/3;if(!high("RESET"+n))s[i]=0;else if(read("TRIG"+n)&&*read("TRIG"+n)-*ground<control/2)s[i]=1;else if(read("THRESH"+n)&&*read("THRESH"+n)-*ground>control)s[i]=0;}}
    }
}
}
