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
#define PROGMEM
#define pgm_read_byte(p) (*reinterpret_cast<const unsigned char*>(p))
using byte = unsigned char;
constexpr int LOW = 0, HIGH = 1, INPUT = 0, OUTPUT = 1, INPUT_PULLUP = 2;
constexpr int D0=16, D1=5, D2=4, D3=0, D4=2, D5=14, D6=12, D7=13, D8=15, A0=17, LED_BUILTIN=2;
namespace thinker {
inline std::uint64_t clockUs = 0;
inline std::array<int,17> modes{};
inline void validatePin(int pin) {
    if (pin < 0 || pin > 16 || (pin >= 6 && pin <= 11)) throw std::invalid_argument("Unavailable ESP8266 GPIO");
}
inline void advance(std::uint64_t us) {
    if (us > std::numeric_limits<std::uint64_t>::max() - clockUs) throw std::overflow_error("Clock overflow");
    do {
        const auto step = std::min<std::uint64_t>(us, 50000);
        std::this_thread::sleep_for(std::chrono::microseconds(step));
        clockUs += step; us -= step;
        std::cout << "TIME " << clockUs << std::endl;
    } while (us);
}
inline int request(const std::string& line) {
    std::cout << line << std::endl;
    int answer = 0;
    if (!(std::cin >> answer)) throw std::runtime_error("Simulation host disconnected");
    return answer;
}
}
inline void pinMode(int pin, int mode) {
    thinker::validatePin(pin);
    if (mode < INPUT || mode > INPUT_PULLUP || (pin == 16 && mode == INPUT_PULLUP)) throw std::invalid_argument("Unsupported pin mode");
    thinker::modes.at(pin)=mode;
    std::cout << "MODE " << pin << ' ' << mode << std::endl;
}
inline void digitalWrite(int pin, int value) {
    thinker::validatePin(pin);
    if (thinker::modes.at(pin) != OUTPUT || (value != HIGH && value != LOW)) throw std::invalid_argument("digitalWrite requires OUTPUT and HIGH/LOW");
    std::cout << "WRITE " << pin << ' ' << value << std::endl;
}
inline int digitalRead(int pin) { thinker::validatePin(pin); return thinker::request("READ " + std::to_string(pin)); }
inline int analogRead(int pin) { if (pin != A0) throw std::invalid_argument("ESP8266 only has A0"); return thinker::request("ADC"); }
inline unsigned long millis() { return static_cast<std::uint32_t>(thinker::clockUs / 1000); }
inline unsigned long micros() { return static_cast<std::uint32_t>(thinker::clockUs); }
inline void delay(unsigned long ms) { thinker::advance(static_cast<std::uint64_t>(ms)*1000); }
inline void delayMicroseconds(unsigned int us) { thinker::advance(us); }
inline void yield() { delayMicroseconds(0); }
struct SerialPort {
    void begin(unsigned long) {}
    template<class T> void print(const T& v) { std::cerr << v; }
    template<class T> void println(const T& v) { std::cerr << v << '\n'; }
    void println() { std::cerr << '\n'; }
};
inline SerialPort Serial;
