#include "Arduino.h"
#include "Wire.h"
#include "SPI.h"
#include "LiquidCrystal_I2C.h"
#include "Servo.h"
#include "Adafruit_SSD1306.h"
#include "Adafruit_PN532.h"
#include "RF24.h"
#include "IRremote.hpp"
#include "RCSwitch.h"
#include <sstream>
#include <stdexcept>
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
std::vector<uint8_t> lastFrame(const std::string& messages){const auto start=messages.rfind("LCD_FRAME ");check(start!=std::string::npos,"LCD emitted a frame");const auto hexStart=messages.find(' ',start+10)+1;const auto end=messages.find('\n',hexStart);const auto hex=messages.substr(hexStart,end-hexStart);check(hex.size()==2048,"LCD frame length");std::vector<uint8_t> bytes;for(std::size_t i=0;i<hex.size();i+=2)bytes.push_back(std::stoi(hex.substr(i,2),nullptr,16));return bytes;}
int main(){auto* old=std::cout.rdbuf();try{
    thinker::standalone=true;std::ostringstream messages;std::cout.rdbuf(messages.rdbuf());
    Stream* serial=&Serial;Serial.begin(115200);serial->setTimeout(0);check(serial->readString().empty()&&serial->getTimeout()==0,"Serial supports Stream API");Serial.write(0);Serial.write("hello",5);check(messages.str().find("SERIAL 00")!=std::string::npos,"Serial numeric zero writes a byte");
    Wire.begin();check(Wire.sda()==SDA&&Wire.scl()==SCL,"Wire board defaults");Wire.setTimeout(2);const auto before=thinker::clockUs;char c;check(Wire.readBytes(&c,1)==0&&thinker::clockUs-before==2000,"Wire Stream timeout advances simulated time");
    SPI.begin();check(SPI.ready(),"SPI board defaults initialize");
    LiquidCrystal lcd(D1,D2,D3,D4,D5,D6);lcd.begin(20,4);lcd.command(LCD_SETDDRAMADDR);lcd.write('H');auto frame=lastFrame(messages.str());check(frame[0]==0x7f&&frame[1]==8&&frame[4]==0x7f,"LCD character bitmap");
    lcd.command(LCD_DISPLAYCONTROL);frame=lastFrame(messages.str());check(std::all_of(frame.begin(),frame.end(),[](auto p){return p==0;}),"LCD command turns display off");
    lcd.command(LCD_DISPLAYCONTROL|LCD_DISPLAYON);lcd.command(LCD_SETCGRAMADDR);for(int i=0;i<8;++i)lcd.write(uint8_t(31));lcd.command(LCD_SETDDRAMADDR|20);lcd.write(0);frame=lastFrame(messages.str());check((frame[256]&0xf0)==0xf0&&(frame[384]&0xf)==0xf,"LCD CGRAM command and row-three address");
    lcd.command(LCD_CLEARDISPLAY);lcd.setCursor(39,0);lcd.write('H');lcd.write('H');frame=lastFrame(messages.str());check((frame[128]&0xfc)==0xfc,"LCD DDRAM wraps from first 40-character bank to second");
    lcd.command(LCD_CURSORSHIFT|LCD_DISPLAYMOVE|LCD_MOVELEFT);lcd.command(LCD_ENTRYMODESET|LCD_ENTRYRIGHT);lcd.command(LCD_RETURNHOME);
    lcd.begin(16,2);lcd.print("H");frame=lastFrame(messages.str());check(frame[0]==0x7f,"LCD reinitialization resets address and CGRAM state");
    LiquidCrystal_I2C i2c(0x27,16,2);i2c.init();i2c.setBacklight(255);check(messages.str().find("LCD_LIGHT ")!=std::string::npos,"public I2C backlight API");
    Servo servo;servo.write(30);const auto pulse=servo.readMicroseconds();servo.attach(D5);check(servo.attached()&&servo.readMicroseconds()==pulse&&servo.read()>=29&&servo.read()<=30,"Servo preserves pre-attach position");servo.detach();
    Adafruit_SSD1306 small(128,32),large(128,64);check(small.begin()&&large.begin(),"OLED standard default begin API");check(messages.str().find("OLED_BEGIN 60 32")!=std::string::npos&&messages.str().find("OLED_BEGIN 61 64")!=std::string::npos,"OLED standard automatic addresses");
    bool rejected=false;try{Adafruit_PN532 nfc(4,5,nullptr);}catch(const std::invalid_argument&){rejected=true;}check(rejected,"PN532 null bus rejected");RF24 radio(4,5);rejected=false;try{radio.begin(nullptr);}catch(const std::invalid_argument&){rejected=true;}check(rejected,"RF24 null bus rejected");
    std::cout.rdbuf(old);std::cout<<"Bundled SDK compatibility passed for this board\n";return 0;
}catch(const std::exception& e){std::cout.rdbuf(old);std::cerr<<e.what()<<'\n';return 1;}}
