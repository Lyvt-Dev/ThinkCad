#pragma once
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <thread>
#include <map>
#include <functional>
#include "pgmspace.h"
#include "Print.h"
#ifndef ARDUINO
#define ARDUINO 10819
#endif
#ifndef F
#define F(text) PSTR(text)
#endif
using byte = unsigned char;
constexpr int LOW = 0, HIGH = 1, INPUT = 0, OUTPUT = 1, INPUT_PULLUP = 2;
#if defined(THINKERCAD_ESP32_S3)
constexpr int D0=0,D1=1,D2=2,D3=3,D4=4,D5=5,D6=6,D7=7,D8=8,A0=1,LED_BUILTIN=48,SDA=8,SCL=9;
#elif defined(THINKERCAD_ESP32)
constexpr int D0=0,D1=1,D2=2,D3=3,D4=4,D5=5,D6=12,D7=13,D8=14,A0=36,LED_BUILTIN=2,SDA=21,SCL=22;
#else
constexpr int D0=16,D1=5,D2=4,D3=0,D4=2,D5=14,D6=12,D7=13,D8=15,A0=17,LED_BUILTIN=2,SDA=4,SCL=5;
#endif
namespace thinker {
inline std::uint64_t clockUs = 0;
inline bool standalone = false;
inline std::map<const void*,std::function<void()>> timeObservers;
inline std::array<int,49> modes{};
inline void validatePin(int pin) {
#if defined(THINKERCAD_ESP32_S3)
    if(!((pin>=0&&pin<=21)||(pin>=35&&pin<=48)))throw std::invalid_argument("Unavailable ESP32-S3 GPIO");
#elif defined(THINKERCAD_ESP32)
    constexpr int pins[]={0,1,2,3,4,5,12,13,14,15,16,17,18,19,21,22,23,25,26,27,32,33,34,35,36,39};if(std::find(std::begin(pins),std::end(pins),pin)==std::end(pins))throw std::invalid_argument("Unavailable ESP32 GPIO");
#else
    if(pin<0||pin>16||(pin>=6&&pin<=11))throw std::invalid_argument("Unavailable ESP8266 GPIO");
#endif
}
inline int request(const std::string& line);
inline void advance(std::uint64_t us) {
    if (us > std::numeric_limits<std::uint64_t>::max() - clockUs) throw std::overflow_error("Clock overflow");
    do {
        const int speed = request("SPEED"); // Speed multiplier in thousandths; refreshed while running.
        if (speed < 100 || speed > 10000) throw std::runtime_error("Invalid simulation speed");
        const auto step = std::min<std::uint64_t>(us, 50ull * speed);
        std::this_thread::sleep_for(std::chrono::microseconds(step * 1000 / speed));
        clockUs += step;us -= step;for(const auto& observer:timeObservers)observer.second();
        std::cout << "TIME " << clockUs << std::endl;
    } while (us);
}
inline int request(const std::string& line) {
    std::cout << line << std::endl;
    if(standalone){if(line=="SPEED")return 1000;if(line.starts_with("OLED_BEGIN "))return 1;return 0;}
    int answer = 0;
    if (!(std::cin >> answer)) throw std::runtime_error("Simulation host disconnected");
    return answer;
}
}
inline void pinMode(int pin, int mode) {
    thinker::validatePin(pin);
    if (mode < INPUT || mode > INPUT_PULLUP || (
#if defined(THINKERCAD_ESP32_S3)
    pin==46&&mode!=INPUT
#elif defined(THINKERCAD_ESP32)
    pin>=34&&mode!=INPUT
#else
    pin==16&&mode==INPUT_PULLUP
#endif
    )) throw std::invalid_argument("Unsupported pin mode");
    thinker::modes.at(pin)=mode;
    std::cout << "MODE " << pin << ' ' << mode << std::endl;
}
inline void digitalWrite(int pin, int value) {
    thinker::validatePin(pin);
    if (thinker::modes.at(pin) != OUTPUT || (value != HIGH && value != LOW)) throw std::invalid_argument("digitalWrite requires OUTPUT and HIGH/LOW");
    std::cout << "WRITE " << pin << ' ' << value << std::endl;
}
inline int digitalRead(int pin) { thinker::validatePin(pin); return thinker::request("READ " + std::to_string(pin)); }
inline int analogRead(int pin) {
#if defined(THINKERCAD_ESP32_S3)
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
inline void yield() { delayMicroseconds(0); }
struct SerialPort : Print {
    using Print::write;
    void begin(unsigned long) {} explicit operator bool()const{return true;}
    std::size_t write(std::uint8_t value)override{std::cerr.put(static_cast<char>(value));return 1;}
};
inline SerialPort Serial;
