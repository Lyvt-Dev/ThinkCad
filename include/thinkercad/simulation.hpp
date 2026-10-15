#pragma once
#include <array>
#include <cstdint>
#include <functional>
#include <map>
#include <queue>
#include <string>
#include <vector>

namespace thinkercad {
using Microseconds = std::uint64_t;
enum class PinMode { Input, InputPullup, Output };
enum class Level { Floating, Low, High, Contention };
struct Pin { PinMode mode = PinMode::Input; bool output = false; Level external = Level::Floating; unsigned frequency = 0; double duty = 0.5; bool pwm = false; };

class Component {
public:
    explicit Component(std::string id) : id_(std::move(id)) {}
    virtual ~Component() = default;
    const std::string& id() const { return id_; }
    virtual void reset() = 0;
private:
    std::string id_;
};

// A digital net resolves strong drivers first; weak pullups cannot override them.
class DigitalNet {
public:
    static Level resolve(const std::vector<Level>& drivers, bool pullup = false);
};
class Board : public Component {
public:
    using Component::Component;
    virtual void pinMode(int gpio, PinMode mode) = 0;
    virtual void digitalWrite(int gpio, bool high) = 0;
    virtual Level digitalRead(int gpio) const = 0;
};
struct BoardProfile {
    std::string id,label;std::vector<int> gpio,adc,inputOnly;double adcMax=1;int adcBits=10,defaultAdc=17,sda=4,scl=5;
    double voltage=3.3; int builtinLed=2;
    bool arduino()const{return id=="arduino-uno"||id=="arduino-nano";}
    std::string pinName(int gpio)const;
    int pinNumber(const std::string& name)const;
    static BoardProfile get(const std::string& id);
};
class Esp8266Board final : public Board {
public:
    Esp8266Board();
    void configure(const std::string& model);
    const BoardProfile& profile()const{return profile_;}
    static constexpr std::array<int, 9> nodeMcuGpio{16, 5, 4, 0, 2, 14, 12, 13, 15};
    static constexpr double supplyVoltage = 3.3;
    void reset() override;
    void pinMode(int gpio, PinMode mode) override;
    void digitalWrite(int gpio, bool high) override;
    Level digitalRead(int gpio) const override;
    void tone(int gpio, unsigned frequency);
    void servoWrite(int gpio,unsigned pulse);
    void analogWrite(int gpio,unsigned value,unsigned range=255,unsigned frequency=1000);
    void driveInput(int gpio, Level level);
    void setAdcVoltage(double volts); // Bare ESP8266 A0: 0..1 V, not NodeMCU divider.
    int analogRead() const;
    double adcVoltage() const { return adcVoltage_; }
    const Pin& pin(int gpio) const;
private:
    BoardProfile profile_=BoardProfile::get("esp8266");
    std::map<int, Pin> pins_;
    double adcVoltage_ = 0;
};
class Led final : public Component {
public:
    Led(std::string id, double resistance = 220, double forwardVoltage = 2.0);
    void reset() override { current_ = 0; }
    void apply(double anodeVoltage, double cathodeVoltage);
    double currentAmps() const { return current_; }
    double brightness() const;
private:
    double resistance_, forwardVoltage_, current_ = 0;
};
struct Frame {
    int width = 128, height = 64;
    std::vector<std::uint8_t> pixels;
    Frame(int width = 128, int height = 64);
    void setPixel(int x, int y, bool on);
    bool pixel(int x, int y) const;
};
class Display : public Component {
public:
    using Component::Component;
    virtual void present(const Frame& frame) = 0;
    virtual const Frame& frame() const = 0;
};
class OledDisplay final : public Display {
public:
    OledDisplay();
    void reset() override;
    void present(const Frame& frame) override;
    const Frame& frame() const override { return visible_; }
    std::uint64_t revision() const { return revision_; }
    double brightness()const{return brightness_;}
    void setBrightness(double value);
    static Microseconds transferDuration(std::size_t bytes, unsigned busHz = 400000);
private:
    Frame visible_;
    std::uint64_t revision_ = 0;
    double brightness_=1;
};
class Simulator {
public:
    void schedule(Microseconds at, std::function<void()> action);
    void runUntil(Microseconds until, std::size_t eventBudget = 100000);
    void reset();
    Microseconds now() const { return now_; }
    std::size_t pendingEvents() const { return events_.size(); }
private:
    struct Event { Microseconds at; std::uint64_t sequence; std::function<void()> action; };
    struct Later { bool operator()(const Event& a, const Event& b) const { return a.at > b.at || (a.at == b.at && a.sequence > b.sequence); } };
    std::priority_queue<Event, std::vector<Event>, Later> events_;
    Microseconds now_ = 0;
    std::uint64_t sequence_ = 0;
};
class Animation {
public:
    void addFrame(Frame frame, Microseconds duration);
    // Schedules one finite playback. Copies frames so events never reference a destroyed Animation.
    void enqueue(Simulator& sim, OledDisplay& display, Microseconds start) const;
private:
    struct Entry { Frame frame; Microseconds duration; };
    std::vector<Entry> entries_;
};
} // namespace thinkercad
