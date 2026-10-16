#include "thinkercad/modules.hpp"
#include <algorithm>
#include <cmath>
#include <set>
#include <stdexcept>
namespace thinkercad {
namespace {
std::map<std::string,ModuleDefinition>& models(){
    static auto values=[] {
        std::map<std::string,ModuleDefinition> result;
        auto add=[&](std::string id,std::string label,std::string bus,std::string implementation,std::vector<std::string> pins,std::vector<ModuleParameter> params,double w,double h,double low,double high,int address=0,unsigned clock=400000){
            ModuleDefinition d;d.id=std::move(id);d.label=std::move(label);d.bus=std::move(bus);d.implementation=std::move(implementation);d.pins=std::move(pins);d.parameters=std::move(params);d.width=w;d.height=h;d.minVoltage=low;d.maxVoltage=high;d.address=address;d.maxClock=clock;d.builtin=true;if(d.bus=="i2c")d.pullups={{"SDA",4700},{"SCL",4700}};if(d.implementation=="dht22")d.pullups={{"DATA",4700}};if(d.implementation=="ds18b20")d.pullups={{"DQ",4700}};if(d.implementation=="mfrc522"){d.pullups={{"RST",10000}};d.idleCurrent=.025;}if(d.implementation=="ws2812")d.idleCurrent=.008;
            if(d.bus=="i2c"||d.bus=="pulse"||d.bus=="single-wire"||d.bus=="onewire"||d.implementation=="mfrc522"){const double pitch=d.implementation=="ds18b20"?1.27:2.54;for(std::size_t i=0;i<d.pins.size();++i)d.pinPositions[d.pins[i]]={d.width/2+(double(i)-(d.pins.size()-1)/2.)*pitch,d.height};}
            result.emplace(d.id,d);
        };
        const std::vector<ModuleParameter> environment{{"temperature","Temperature","°C",25,-40,85},{"humidity","Humidity","%",50,0,100},{"pressure","Pressure","hPa",1013.25,300,1100}};
        add("bme280","BME280 · I2C","i2c","bme280",{"VCC","GND","SDA","SCL"},environment,11.5,15,1.71,3.6,0x76);
        add("bme280-spi","BME280 · SPI","spi","bme280",{"VCC","GND","SCK","MISO","MOSI","CS"},environment,15,18,1.71,3.6,0,10000000);
        add("mpu6050","MPU6050 · 3.3 V","i2c","mpu6050",{"VCC","GND","SDA","SCL","INT"},{{"accelX","Acceleration X","g",0,-16,16},{"accelY","Acceleration Y","g",0,-16,16},{"accelZ","Acceleration Z","g",1,-16,16},{"gyroX","Angular velocity X","°/s",0,-2000,2000},{"gyroY","Angular velocity Y","°/s",0,-2000,2000},{"gyroZ","Angular velocity Z","°/s",0,-2000,2000},{"temperature","Temperature","°C",25,-40,85}},20,16,2.375,3.46,0x68);
        add("dht22","DHT22 / AM2302","single-wire","dht22",{"VCC","DATA","GND"},{{"temperature","Temperature","°C",25,-40,80},{"humidity","Humidity","%",50,0,100}},15.1,25,3.3,6);
        add("hcsr04","HC-SR04","pulse","hcsr04",{"VCC","TRIG","ECHO","GND"},{{"distance","Distance","cm",100,2,400}},45,20,4.5,5.5);
        add("ds18b20","DS18B20","onewire","ds18b20",{"GND","DQ","VCC"},{{"temperature","Temperature","°C",25,-55,125}},4.9,10.2,3,5.5);
        add("ws2812","WS2812B · RGB strip","neopixel","ws2812",{"VCC","DIN","GND","DOUT"},{{"pixels","Pixel count","",8,1,64}},40,10,3.5,5.3,0,800000);
        add("l298n","L298N · dual H bridge","bridge","l298n",{"VS","VCC","GND","ENA","IN1","IN2","OUT1","OUT2","ENB","IN3","IN4","OUT3","OUT4"},{},43,43,4.5,5.5);
        add("mfrc522","RFID RC522 · SPI","spi","mfrc522",{"VCC","GND","SCK","MISO","MOSI","CS","RST","IRQ"},{},40,60,2.5,3.6,0,10000000);
        add("mcp3008","MCP3008 · SPI ADC","spi","mcp3008",{"VCC","VREF","GND","SCK","MISO","MOSI","CS","CH0","CH1","CH2","CH3","CH4","CH5","CH6","CH7"},{},10,20,2.7,5.5,0,1000000);
        return result;
    }();return values;
}
bool identifier(const std::string& text){return !text.empty()&&text.size()<=80&&std::all_of(text.begin(),text.end(),[](unsigned char c){return (c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c=='-'||c=='+';});}
}
const std::map<std::string,ModuleDefinition>& ModuleRegistry::definitions(){return models();}
const ModuleDefinition& ModuleRegistry::get(const std::string& id){const auto it=models().find(id);if(it==models().end())throw std::invalid_argument("Unknown local component model: "+id);return it->second;}
void ModuleRegistry::validate(const ModuleDefinition& d){
    if(!identifier(d.id)||d.label.empty()||d.label.size()>120||d.pins.size()<2||d.pins.size()>32||!identifier(d.power)||!identifier(d.ground))throw std::invalid_argument("Invalid module identity or pins");
    std::set<std::string> pins;for(const auto& pin:d.pins)if(!identifier(pin)||!pins.insert(pin).second)throw std::invalid_argument("Invalid or duplicate module pin");
    for(const auto& [pin,position]:d.pinPositions)if(!pins.contains(pin)||!std::isfinite(position.first)||!std::isfinite(position.second)||position.first<0||position.first>d.width||position.second<0||position.second>d.height)throw std::invalid_argument("Invalid module pin position");
    if(!std::isfinite(d.idleCurrent)||d.idleCurrent<0||d.idleCurrent>2)throw std::invalid_argument("Invalid module idle current");
    for(const auto& [pin,resistance]:d.pullups)if(!pins.contains(pin)||pin==d.power||pin==d.ground||!std::isfinite(resistance)||resistance<330||resistance>100000)throw std::invalid_argument("Invalid module pullup");
    if(!pins.contains(d.power)||!pins.contains(d.ground)||d.power==d.ground)throw std::invalid_argument("Module supply pins are missing");
    if(!std::isfinite(d.width)||!std::isfinite(d.height)||d.width<3||d.width>200||d.height<3||d.height>200||!std::isfinite(d.minVoltage)||!std::isfinite(d.maxVoltage)||d.minVoltage<=0||d.maxVoltage<d.minVoltage||d.maxVoltage>48)throw std::invalid_argument("Invalid module dimensions or voltage range");
    if(d.bus!="i2c"&&d.bus!="spi"&&d.bus!="single-wire"&&d.bus!="pulse"&&d.bus!="onewire"&&d.bus!="neopixel"&&d.bus!="bridge")throw std::invalid_argument("Unsupported module bus");
    if((d.bus=="i2c"&&(!pins.contains("SDA")||!pins.contains("SCL")||d.address<8||d.address>119))||(d.bus=="spi"&&(!pins.contains("SCK")||!pins.contains("MISO")||!pins.contains("MOSI")||!pins.contains("CS"))))throw std::invalid_argument("Missing bus pins or invalid address");
    if(d.spiMode<0||d.spiMode>3||d.maxClock<1||d.maxClock>80000000||d.parameters.size()>32||d.registers.size()>256)throw std::invalid_argument("Invalid module limits");
    std::set<std::string> parameters;for(const auto& p:d.parameters)if(!identifier(p.name)||!parameters.insert(p.name).second||p.label.empty()||p.label.size()>120||p.unit.size()>20||!std::isfinite(p.initial)||!std::isfinite(p.min)||!std::isfinite(p.max)||p.min>p.initial||p.initial>p.max)throw std::invalid_argument("Invalid module parameter");
    for(const auto& p:d.parameters)if(p.registerAddress < -1||p.registerAddress>255||p.bytes<1||p.bytes>4||p.registerAddress+p.bytes>256||!std::isfinite(p.scale)||p.scale==0||std::abs(p.scale)>1e9)throw std::invalid_argument("Invalid parameter register mapping");
    std::set<int> mapped;for(const auto& p:d.parameters)if(p.registerAddress>=0){const double limit=std::ldexp(1.,p.bytes*8-(p.signedValue?1:0));const double low=std::min(p.min*p.scale,p.max*p.scale),high=std::max(p.min*p.scale,p.max*p.scale);if(!std::isfinite(low)||!std::isfinite(high)||low<(p.signedValue?-limit:0)||high>limit-1)throw std::invalid_argument("Parameter exceeds encoded register width");for(int n=0;n<p.bytes;++n)if(!mapped.insert(p.registerAddress+n).second)throw std::invalid_argument("Overlapping parameter registers");}
    for(const auto& [reg,value]:d.registers)if(reg>255||value>255)throw std::invalid_argument("Module register outside byte range");
    for(auto reg:d.readOnly)if(reg>255)throw std::invalid_argument("Read-only register outside byte range");
}
void ModuleRegistry::install(ModuleDefinition d){validate(d);if(d.builtin||d.implementation!="registers"||(d.bus!="i2c"&&d.bus!="spi"))throw std::invalid_argument("Local packages may only define register devices");const auto old=models().find(d.id);if(old!=models().end())throw std::invalid_argument("Component model already installed: "+d.id);if(models().size()>=256)throw std::invalid_argument("Maximum 256 component models");const auto id=d.id;models().emplace(id,std::move(d));}
void ModuleRegistry::removeLocal(const std::string& id){const auto it=models().find(id);if(it!=models().end()&&!it->second.builtin)models().erase(it);}
}
