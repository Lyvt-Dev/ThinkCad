#include "thinkercad/electronics.hpp"
#include "thinkercad/moduledevices.hpp"
#include <iostream>
#include <cmath>
#include <stdexcept>
using namespace thinkercad;
void check(bool ok,const std::string& message){if(!ok)throw std::runtime_error(message);}
int main(){try{
    for(const auto& [id,model]:ModuleRegistry::definitions()){try{ModuleRegistry::validate(model);}catch(const std::exception& e){throw std::runtime_error(id+": "+e.what());}}
    CircuitDocument doc;doc.part("board").model="arduino-uno";Esp8266Board board;board.configure("arduino-uno");
    auto part=[&](const std::string& model){auto id=doc.addPart(PartKind::Module,0,0);doc.part(id).model=model;return id;};
    auto chip=part("74hc595");doc.connect({chip,"VCC"},{"board","5V"});doc.connect({chip,"GND"},{"board","GND"});
    const std::vector<std::string> controls{"SER","SRCLK","RCLK","SRCLR","OE"};for(int i=0;i<5;++i){doc.connect({chip,controls[i]},{"board","D"+std::to_string(i+2)});board.pinMode(i+2,PinMode::Output);board.digitalWrite(i+2,false);}board.digitalWrite(5,true);
    TransientSimulation transient;auto run=[&]{return transient.evaluate(doc,{{"board",&board}});};run();
    unsigned pattern=0xa5;for(int bit=7;bit>=0;--bit){board.digitalWrite(2,pattern&(1<<bit));board.digitalWrite(3,false);run();board.digitalWrite(3,true);run();}board.digitalWrite(4,true);auto state=run();
    for(int i=0;i<8;++i)check(state.level({chip,"Q"+std::string(1,char('A'+i))})==((pattern&(1<<i))?Level::High:Level::Low),"595 shift/latch bit "+std::to_string(i));
    auto mux=part("cd74hc4067");doc.connect({mux,"VCC"},{"board","5V"});doc.connect({mux,"GND"},{"board","GND"});doc.connect({mux,"EN"},{"board","GND"});for(int i=0;i<4;++i)doc.connect({mux,"S"+std::to_string(i)},{"board",i==1?"5V":"GND"});doc.connect({mux,"C2"},{"board","3V3"});state=run();check(state.voltage.at({mux,"SIG"})&&std::abs(*state.voltage.at({mux,"SIG"})-3.3)<.01,"4067 bidirectional analog channel");
    ModuleDevices devices;auto dac=part("mcp4725");doc.connect({dac,"VCC"},{"board","5V"});doc.connect({dac,"GND"},{"board","GND"});devices.i2c(doc.part(dac),{0x40,0x80,0x00},-1,0);state=run();transient.setPeripheralSignals(dac,devices.outputSignals(doc.part(dac),state));state=run();check(state.voltage.at({dac,"VOUT"})&&std::abs(*state.voltage.at({dac,"VOUT"})-2.5)<.001,"MCP4725 12-bit code reaches physical analog output");
    auto spiDac=part("mcp4822");doc.connect({spiDac,"VCC"},{"board","5V"});doc.connect({spiDac,"GND"},{"board","GND"});doc.connect({spiDac,"LDAC"},{"board","GND"});devices.select(doc.part(spiDac),true);devices.spi(doc.part(spiDac),0x38,0);devices.spi(doc.part(spiDac),0x00,0);state=run();transient.setPeripheralSignals(spiDac,devices.outputSignals(doc.part(spiDac),state));state=run();check(state.voltage.at({spiDac,"VOUTA"})&&std::abs(*state.voltage.at({spiDac,"VOUTA"})-1.024)<.001,"MCP4822 SPI frame and internal reference");
    auto dds=part("ad9833");doc.connect({dds,"VCC"},{"board","5V"});doc.connect({dds,"GND"},{"board","GND"});devices.select(doc.part(dds),true);auto word=[&](unsigned value){devices.spi(doc.part(dds),value>>8,0);devices.spi(doc.part(dds),value&255,0);};unsigned tuning=unsigned(std::llround(440.*268435456/25000000));word(0x2100);word(0x4000|(tuning&0x3fff));word(0x4000|(tuning>>14));word(0x2000);state=run();auto ddsSignals=devices.outputSignals(doc.part(dds),state);check(ddsSignals.size()==1&&std::abs(ddsSignals[0].frequency-440)<.1,"AD9833 control/frequency words");
    auto amp=part("tl072");auto positive=doc.addPart(PartKind::PowerSupply,0,0),negative=doc.addPart(PartKind::PowerSupply,0,0);doc.part(positive).value=doc.part(negative).value=12;doc.connect({positive,"-"},{"board","GND"});doc.connect({negative,"+"},{"board","GND"});doc.connect({positive,"+"},{amp,"V+"});doc.connect({negative,"-"},{amp,"V-"});doc.connect({amp,"IN+1"},{"board","3V3"});doc.connect({amp,"OUT1"},{amp,"IN-1"});doc.connect({amp,"IN+2"},{"board","GND"});doc.connect({amp,"IN-2"},{"board","GND"});state=run();check(state.errors.empty(),state.errors.empty()?"":state.errors[0]);check(state.voltage.at({amp,"OUT1"})&&std::abs(*state.voltage.at({amp,"OUT1"})-3.3)<.001,"TL072 closed-loop voltage follower");
    auto legacyAmp=part("lm741");doc.connect({positive,"+"},{legacyAmp,"V+"});doc.connect({negative,"-"},{legacyAmp,"V-"});doc.connect({legacyAmp,"IN+"},{"board","3V3"});doc.connect({legacyAmp,"OUT"},{legacyAmp,"IN-"});state=run();check(state.errors.empty(),state.errors.empty()?"":state.errors[0]);check(std::abs(*state.voltage.at({legacyAmp,"OUT"})-3.3)<.001,"LM741 closed-loop follower converges");
    auto bread=doc.addPart(PartKind::Breadboard30,0,0);for(int columns:{17,30,63}){doc.part(bread).parameters={{"columns",double(columns)},{"rails",1}};doc.validate();check(CircuitDocument::pins(doc.part(bread)).size()==std::size_t(columns*10+breadboardRailContacts(doc.part(bread))*4),"breadboard contacts");check(doc.connected({bread,"a1"},{bread,"e1"}),"breadboard connected column");check(!doc.connected({bread,"a1"},{bread,"f1"}),"breadboard isolated trench");}
    {
        CircuitDocument ladder;ladder.part("board").model="arduino-uno";
        auto id=ladder.addPart(PartKind::Module,0,0);ladder.part(id).model="r2r-dac";
        ladder.connect({id,"GND"},{"board","GND"});
        for(int bit=0;bit<8;++bit)ladder.connect({id,"D"+std::to_string(bit)},{"board",bit==7?"5V":"GND"});
        auto state=ladder.solve({{"board",&board}});check(std::abs(*state.voltage.at({id,"OUT"})-2.5)<.001,"R-2R MSB half-scale voltage");
        auto load=ladder.addPart(PartKind::Resistor,0,0);ladder.part(load).value=1000;
        ladder.connect({load,"1"},{id,"OUT"});ladder.connect({load,"2"},{"board","GND"});
        state=ladder.solve({{"board",&board}});check(std::abs(*state.voltage.at({id,"OUT"})-2.5/11)<.001,"R-2R finite 10k output impedance under 1k load");
    }
    {
        auto id=part("ad9850");doc.connect({id,"VCC"},{"board","5V"});doc.connect({id,"GND"},{"board","GND"});
        doc.connect({id,"FQ_UD"},{"board","D7"});board.pinMode(7,PinMode::Output);board.digitalWrite(7,false);
        devices.select(doc.part(id),true);std::uint32_t tuning=std::uint32_t(std::llround(1000.*4294967296/125000000));
        for(int i=0;i<5;++i){std::uint8_t byte=i==4?0:std::uint8_t(tuning>>(i*8)),wire=0;for(int bit=0;bit<8;++bit)wire|=((byte>>bit)&1)<<(7-bit);devices.spi(doc.part(id),wire,0);}
        state=run();auto outputs=devices.outputSignals(doc.part(id),state);check(outputs[0].frequency==0,"AD9850 waits for FQ_UD");
        board.digitalWrite(7,true);state=run();outputs=devices.outputSignals(doc.part(id),state);check(std::abs(outputs[0].frequency-1000)<.1,"AD9850 40-bit LSB-first update");
    }
    {
        CircuitDocument pads;auto id=pads.addPart(PartKind::Module,0,0);pads.part(id).model="drum-pad";ReactiveState memory;CircuitState inputs;
        advanceAnalogDevices(pads,inputs,memory,0);check(memory.analog[id].outL==0,"Unstruck drum pad is silent");
        pads.part(id).parameters["strike"]=50;advanceAnalogDevices(pads,inputs,memory,1000);check(std::abs(memory.analog[id].outL-2.5)<.001,"Drum-pad velocity sets impulse peak");
        advanceAnalogDevices(pads,inputs,memory,41000);check(std::abs(memory.analog[id].outL)<.02,"Held strike decays rather than generating DC");
        pads.part(id).parameters["strike"]=0;advanceAnalogDevices(pads,inputs,memory,42000);pads.part(id).parameters["strike"]=100;advanceAnalogDevices(pads,inputs,memory,43000);check(std::abs(memory.analog[id].outL-5)<.001,"Released pad can retrigger at a new velocity");
    }
    {
        CircuitPart touch;touch.id="touch";touch.kind=PartKind::Module;touch.model="mpr121";touch.parameters["touchMask"]=5;
        devices.i2c(touch,{0},-1,0);check(devices.i2c(touch,{},2,0)==std::vector<std::uint8_t>({0,0}),"MPR121 electrodes disabled after reset");
        devices.i2c(touch,{0x5e,12},-1,0);devices.i2c(touch,{0},-1,0);check(devices.i2c(touch,{},2,0)==std::vector<std::uint8_t>({5,0}),"MPR121 enabled electrode touch bitmask");
        devices.i2c(touch,{0x80,0x63},-1,0);devices.i2c(touch,{0x5e},-1,0);check(devices.i2c(touch,{},1,0)[0]==0,"MPR121 soft reset clears electrode configuration");
        CircuitPart volume;volume.id="volume";volume.kind=PartKind::Module;volume.model="pt2258";
        devices.i2c(volume,{0xc0,0xd2,0xe3},-1,0);auto outputs=devices.outputSignals(volume,{});check(outputs.size()==6&&std::abs(outputs[0].gain-std::pow(10.,-23./20))<1e-9,"PT2258 master tens and ones attenuation commands");
        devices.i2c(volume,{0xf9},-1,0);check(devices.outputSignals(volume,{})[0].gain==0,"PT2258 mute affects every output");
    }
    {
        CircuitPart codec;codec.id="midi-codec";codec.kind=PartKind::Module;codec.model="vs1053b";codec.parameters["midi"]=1;
        CircuitState pins;pins.voltage[{codec.id,"VCC"}]=3.3;pins.voltage[{codec.id,"GND"}]=0;pins.voltage[{codec.id,"RESET"}]=3.3;
        devices.receiveMidi(codec,{0x90,69,127},0);auto outputs=devices.outputSignals(codec,pins,10000);
        check(outputs.size()==3&&outputs[1].pcm&&!outputs[1].pcm->empty(),"VS1053 MIDI note creates actual stereo PCM");
        double energy=0;for(float value:*outputs[1].pcm)energy+=value*value;check(energy>.01,"VS1053 MIDI output contains audible energy");
        devices.receiveMidi(codec,{0x80,69,0},20000);outputs=devices.outputSignals(codec,pins,300000);energy=0;for(float value:*outputs[1].pcm)energy+=value*value;check(energy<1e-8,"VS1053 MIDI note-off finishes its release");
    }
    {
        CircuitPart adder;adder.id="adder";adder.kind=PartKind::Module;adder.model="74hc283";
        for(int a=0;a<16;++a)for(int b=0;b<16;++b)for(int carry=0;carry<2;++carry){
            std::map<std::string,double> pins{{"VCC",5},{"GND",0},{"C0",carry*5.}};
            for(int bit=0;bit<4;++bit){pins["A"+std::to_string(bit+1)]=(a&(1<<bit))?5:0;pins["B"+std::to_string(bit+1)]=(b&(1<<bit))?5:0;}
            auto outputs=electronicBranches(adder,[&](const auto& pin)->std::optional<double>{auto it=pins.find(pin);return it==pins.end()?std::nullopt:std::optional<double>(it->second);},nullptr,0);
            int sum=0;for(const auto& output:outputs)if(output.offset>2.5){if(output.a=="C4")sum|=16;else if(output.a.starts_with("S"))sum|=1<<(output.a.back()-'1');}
            check(sum==a+b+carry,"74HC283 exhaustive 4-bit sum and carry truth table");
        }
        CircuitPart flip;flip.id="flip";flip.kind=PartKind::Module;flip.model="74hc74";
        auto outputs=electronicBranches(flip,[](const auto& pin)->std::optional<double>{return pin=="VCC"?5.:0.;},nullptr,0);
        for(const auto& output:outputs)check(output.offset>2.5,"74HC74 simultaneous active preset/clear forces both outputs HIGH");
    }
    std::cout<<"Electronic catalog, 74HC595, CD74HC4067 and breadboards passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
