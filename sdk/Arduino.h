#pragma once
#include "debugio.hpp"
#include "hostsleep.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <math.h>
#include <stdexcept>
#include <string>
#include <thread>
#include <map>
#include <functional>
#include "pgmspace.h"
#include "Print.h"
#include "Stream.h"
#ifndef ARDUINO
#define ARDUINO 10819
#endif
#ifndef F
#define F(text) (reinterpret_cast<const __FlashStringHelper*>(PSTR(text)))
#endif
using byte = unsigned char;
template<class Value,class Lower,class Upper>
constexpr auto constrain(Value value,Lower lower,Upper upper){
    using Result=std::common_type_t<Value,Lower,Upper>;
    return std::clamp<Result>(static_cast<Result>(value),static_cast<Result>(lower),static_cast<Result>(upper));
}
constexpr int LOW = 0, HIGH = 1, INPUT = 0, OUTPUT = 1, INPUT_PULLUP = 2;
#if defined(THINKERCAD_GENERIC_BOARD)
constexpr int D0=0,D1=1,D2=2,D3=3,D4=4,D5=5,D6=6,D7=7,D8=8;
constexpr int A0=THINKERCAD_BOARD_ADC,A1=A0+1,A2=A0+2,A3=A0+3,A4=A0+4,A5=A0+5,A6=A0+6,A7=A0+7;
constexpr int LED_BUILTIN=THINKERCAD_BOARD_LED,SDA=THINKERCAD_BOARD_SDA,SCL=THINKERCAD_BOARD_SCL;
#if defined(THINKERCAD_STM32F103)
constexpr int PA0=0,PA1=1,PA2=2,PA3=3,PA4=4,PA5=5,PA6=6,PA7=7,PA8=8,PA9=9,PA10=10,PA11=11,PA12=12,PA13=13,PA14=14,PA15=15;
constexpr int PB0=16,PB1=17,PB2=18,PB3=19,PB4=20,PB5=21,PB6=22,PB7=23,PB8=24,PB9=25,PB10=26,PB11=27,PB12=28,PB13=29,PB14=30,PB15=31,PC13=45,PC14=46,PC15=47;
#endif
#elif defined(THINKERCAD_ARDUINO_UNO) || defined(THINKERCAD_ARDUINO_NANO)
constexpr int D0=0,D1=1,D2=2,D3=3,D4=4,D5=5,D6=6,D7=7,D8=8,D9=9,D10=10,D11=11,D12=12,D13=13;
constexpr int A0=14,A1=15,A2=16,A3=17,A4=18,A5=19,LED_BUILTIN=13,SDA=18,SCL=19;
#if defined(THINKERCAD_ARDUINO_NANO)
constexpr int A6=20,A7=21;
#endif
#elif defined(THINKERCAD_ESP32_S3)
constexpr int D0=0,D1=1,D2=2,D3=3,D4=4,D5=5,D6=6,D7=7,D8=8,A0=1,LED_BUILTIN=48,SDA=8,SCL=9;
#elif defined(THINKERCAD_ESP32)
constexpr int D0=0,D1=1,D2=2,D3=3,D4=4,D5=5,D6=12,D7=13,D8=14,A0=36,LED_BUILTIN=2,SDA=21,SCL=22;
#else
constexpr int D0=16,D1=5,D2=4,D3=0,D4=2,D5=14,D6=12,D7=13,D8=15,A0=17,LED_BUILTIN=2,SDA=4,SCL=5;
#endif
#if defined(THINKERCAD_GENERIC_BOARD)
constexpr int SCK=THINKERCAD_BOARD_SPI_SCK,MISO=THINKERCAD_BOARD_SPI_MISO,MOSI=THINKERCAD_BOARD_SPI_MOSI,SS=THINKERCAD_BOARD_SPI_CS;
#elif defined(THINKERCAD_ARDUINO_UNO)||defined(THINKERCAD_ARDUINO_NANO)
constexpr int SCK=13,MISO=12,MOSI=11,SS=10;
#elif defined(THINKERCAD_ESP32_S3)
constexpr int SCK=12,MISO=13,MOSI=11,SS=10;
#elif defined(THINKERCAD_ESP32)
constexpr int SCK=18,MISO=19,MOSI=23,SS=5;
#else
constexpr int SCK=14,MISO=12,MOSI=13,SS=15;
#endif
namespace thinker {
inline std::uint64_t clockUs = 0;
inline bool standalone = false;
inline std::map<const void*,std::function<void()>> timeObservers;
inline std::array<int,49> modes{};
inline std::map<int,std::uint64_t> toneDeadlines;
inline void validatePin(int pin) {
#if defined(THINKERCAD_GENERIC_BOARD)
    if(pin<0||pin>=49||!(THINKERCAD_BOARD_GPIO_MASK&(1ull<<pin)))throw std::invalid_argument("Unavailable board GPIO");
#elif defined(THINKERCAD_ARDUINO_UNO) || defined(THINKERCAD_ARDUINO_NANO)
    if(pin<0||pin>19)throw std::invalid_argument("Unavailable Arduino digital pin");
#elif defined(THINKERCAD_ESP32_S3)
    if(!((pin>=0&&pin<=21)||(pin>=35&&pin<=48)))throw std::invalid_argument("Unavailable ESP32-S3 GPIO");
#elif defined(THINKERCAD_ESP32)
    constexpr int pins[]={0,1,2,3,4,5,12,13,14,15,16,17,18,19,21,22,23,25,26,27,32,33,34,35,36,39};if(std::find(std::begin(pins),std::end(pins),pin)==std::end(pins))throw std::invalid_argument("Unavailable ESP32 GPIO");
#else
    if(pin<0||pin>16||(pin>=6&&pin<=11))throw std::invalid_argument("Unavailable ESP8266 GPIO");
#endif
}
inline int request(const std::string& line);
inline bool interruptsRegistered();
inline void dispatchInterrupts();
inline void advance(std::uint64_t us) {
    if (us > std::numeric_limits<std::uint64_t>::max() - clockUs) throw std::overflow_error("Clock overflow");
    do {
        const int speed = request("SPEED"); // Speed multiplier in thousandths; refreshed while running.
        if (speed < 100 || speed > 10000) throw std::runtime_error("Invalid simulation speed");
        auto step = std::min<std::uint64_t>(us, 50ull * speed);
#ifdef THINKERCAD_EMULATION
        if(interruptsRegistered())step=std::min<std::uint64_t>(step,1000);
#endif
        for(const auto& [pin,deadline]:toneDeadlines)if(deadline>clockUs)step=std::min(step,deadline-clockUs);
        hostSleep(std::chrono::microseconds(step * 1000 / speed));
        clockUs += step;us -= step;
        std::cout << "TIME " << clockUs << std::endl;
        for(auto it=toneDeadlines.begin();it!=toneDeadlines.end();){
            if(it->second<=clockUs){std::cout << "NOTONE " << it->first << std::endl;it=toneDeadlines.erase(it);}else ++it;
        }
        for(const auto& observer:timeObservers)observer.second();
        dispatchInterrupts();
    } while (us);
}
inline int request(const std::string& line) {
    std::cout << line << std::endl;
    if(standalone){if(line=="SERIAL_READ"||line=="SERIAL_PEEK")return -1;if(line=="SPEED")return 1000;if(line.starts_with("OLED_BEGIN "))return 1;return 0;}
    int answer = 0;
    if (!(std::cin >> answer)) throw std::runtime_error("Simulation host disconnected");
    return answer;
}
}
inline void pinMode(int pin, int mode) {
    thinker::validatePin(pin);
    if (mode < INPUT || mode > INPUT_PULLUP || (
#if defined(THINKERCAD_GENERIC_BOARD)
    mode!=INPUT&&(THINKERCAD_BOARD_INPUT_MASK&(1ull<<pin))
#elif defined(THINKERCAD_ARDUINO_UNO) || defined(THINKERCAD_ARDUINO_NANO)
    false
#elif defined(THINKERCAD_ESP32_S3)
    pin==46&&mode!=INPUT
#elif defined(THINKERCAD_ESP32)
    pin>=34&&mode!=INPUT
#else
    pin==16&&mode==INPUT_PULLUP
#endif
    )) throw std::invalid_argument("Unsupported pin mode");
    thinker::toneDeadlines.erase(pin);
    thinker::modes.at(pin)=mode;
    std::cout << "MODE " << pin << ' ' << mode << std::endl;
    thinker::dispatchInterrupts();
}
inline void digitalWrite(int pin, int value) {
    thinker::validatePin(pin);
    if (thinker::modes.at(pin) != OUTPUT || (value != HIGH && value != LOW)) throw std::invalid_argument("digitalWrite requires OUTPUT and HIGH/LOW");
    thinker::toneDeadlines.erase(pin);
    std::cout << "WRITE " << pin << ' ' << value << std::endl;
    thinker::dispatchInterrupts();
}
inline void noTone(int pin) {
    thinker::validatePin(pin);thinker::toneDeadlines.erase(pin);
    thinker::modes.at(pin)=OUTPUT;
    std::cout << "NOTONE " << pin << std::endl;
}
inline void tone(int pin, unsigned int frequency, unsigned long duration=0) {
    thinker::validatePin(pin);
    if(frequency>20000)throw std::invalid_argument("tone supports frequencies up to 20000 Hz");
    if(!frequency){noTone(pin);return;}
    if(duration>(std::numeric_limits<std::uint64_t>::max()-thinker::clockUs)/1000)
        throw std::overflow_error("Tone duration overflow");
    pinMode(pin,OUTPUT); // Also rejects input-only pins for the selected board.
    if(duration)thinker::toneDeadlines[pin]=thinker::clockUs+std::uint64_t(duration)*1000;
    std::cout << "TONE " << pin << ' ' << frequency << std::endl;
}
// Host PWM model: AVR pin frequencies match the default Arduino timer setup.
namespace thinker {inline unsigned pwmRange=255,pwmFrequency=1000;}
inline void analogWrite(int pin,int value){
    thinker::validatePin(pin);
    if(value<0||static_cast<unsigned>(value)>thinker::pwmRange)throw std::invalid_argument("PWM value outside configured range");
    pinMode(pin,OUTPUT);
    std::cout<<"PWM "<<pin<<' '<<value<<' '<<thinker::pwmRange<<' '<<thinker::pwmFrequency<<std::endl;
}
#if !defined(THINKERCAD_ARDUINO_UNO) && !defined(THINKERCAD_ARDUINO_NANO)
inline void analogWriteRange(unsigned range){if(!range||range>65535)throw std::invalid_argument("PWM range outside 1..65535");thinker::pwmRange=range;}
inline void analogWriteFreq(unsigned frequency){if(!frequency||frequency>20000)throw std::invalid_argument("PWM frequency outside 1..20000 Hz");thinker::pwmFrequency=frequency;}
#endif
inline int digitalRead(int pin) { thinker::validatePin(pin);const int value=thinker::request("READ " + std::to_string(pin));thinker::dispatchInterrupts();return value; }
inline int analogRead(int pin) {
#if defined(THINKERCAD_GENERIC_BOARD)
if(pin<0||pin>=49||!(THINKERCAD_BOARD_ADC_MASK&(1ull<<pin)))throw std::invalid_argument("Unavailable ADC GPIO");return thinker::request("ADC "+std::to_string(pin));
#elif defined(THINKERCAD_ARDUINO_UNO) || defined(THINKERCAD_ARDUINO_NANO)
#if defined(THINKERCAD_ARDUINO_NANO)
constexpr int lastAnalog=21;
#else
constexpr int lastAnalog=19;
#endif
// Like the Arduino AVR core, accept both channel numbers and A0..An constants.
if(pin>=0&&pin<=lastAnalog-14)pin+=14;
if(pin<14||pin>lastAnalog)throw std::invalid_argument("Unavailable Arduino analog pin");
return thinker::request("ADC "+std::to_string(pin));
#elif defined(THINKERCAD_ESP32_S3)
if(pin<1||pin>20)throw std::invalid_argument("Unavailable ADC GPIO");return thinker::request("ADC "+std::to_string(pin));
#elif defined(THINKERCAD_ESP32)
constexpr int adc[]={0,2,4,12,13,14,15,25,26,27,32,33,34,35,36,39};if(std::find(std::begin(adc),std::end(adc),pin)==std::end(adc))throw std::invalid_argument("Unavailable ADC GPIO");return thinker::request("ADC "+std::to_string(pin));
#else
if(pin!=A0)throw std::invalid_argument("ESP8266 only has A0");return thinker::request("ADC");
#endif
 }
inline unsigned long millis() { return static_cast<std::uint32_t>(thinker::clockUs / 1000); }
inline unsigned long micros() { return static_cast<std::uint32_t>(thinker::clockUs); }
inline void delay(unsigned long ms) { thinker::advance(static_cast<std::uint64_t>(ms)*1000); }
inline void delayMicroseconds(unsigned int us) { thinker::advance(us); }
inline void yield() {
#ifdef THINKERCAD_EMULATION
    delayMicroseconds(thinker::interruptsRegistered()?1000:0);
#else
    delayMicroseconds(0);
#endif
}
inline unsigned long pulseIn(uint8_t pin,uint8_t state,unsigned long timeout=1000000){
    thinker::validatePin(pin);if(state>1||timeout>10000000)throw std::invalid_argument("Invalid pulse state or timeout");
    if(thinker::standalone){delayMicroseconds(unsigned(timeout));return 0;}
    const int elapsed=thinker::request("PULSE "+std::to_string(pin)+" "+std::to_string(state)+" "+std::to_string(timeout));int width=0;if(!(std::cin>>width)||elapsed<0||width<0||static_cast<unsigned long>(elapsed)>timeout||width>elapsed)throw std::runtime_error("Invalid pulse response");thinker::advance(unsigned(elapsed));return static_cast<unsigned long>(width);
}
inline unsigned long pulseInLong(uint8_t pin,uint8_t state,unsigned long timeout=1000000){return pulseIn(pin,state,timeout);}
#include "interrupts.hpp"
#include "serial/HardwareSerial.hpp"
struct SerialPort : Stream {
    HardwareSerial wired_{0};
    using Print::write;
    void begin(unsigned long baud,uint32_t format=SERIAL_8N1) {
        binary_=baud==31250;
#ifdef THINKERCAD_EMULATION
        wired_.beginMonitor(baud,format);
#else
        (void)baud;(void)format;
#endif
    } explicit operator bool()const{return true;}
    int available()override{return thinker::request("SERIAL_AVAILABLE");}
    int read()override{return thinker::request("SERIAL_READ");}
    int peek()override{return thinker::request("SERIAL_PEEK");}
    int availableForWrite()override{return 512;}
    void end(){wired_.end();}
    std::size_t write(std::uint8_t value)override{return write(&value,1);}
    std::size_t write(const std::uint8_t* data,std::size_t size)override{if(!data&&size)return 0;constexpr char hex[]="0123456789abcdef";for(std::size_t start=0;start<size;start+=512){std::string encoded;const auto count=std::min<std::size_t>(512,size-start);for(std::size_t i=start;i<start+count;++i){encoded+=hex[data[i]>>4];encoded+=hex[data[i]&15];}std::cout<<"SERIAL "<<encoded<<std::endl;wired_.waitTransmit(count);}if(!binary_)std::cerr.write(reinterpret_cast<const char*>(data),size);return size;}
private:
    bool binary_=false;
protected:
    uint64_t streamClock()const override{return thinker::clockUs;}
    void streamWait(uint64_t us)override{thinker::advance(us);}
};
inline SerialPort Serial;
