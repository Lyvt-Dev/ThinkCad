#include "thinkercad/simulation.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
namespace thinkercad {
Level DigitalNet::resolve(const std::vector<Level>& drivers, bool pullup) {
    bool low = false, high = false;
    for (auto d : drivers) {
        if (d == Level::Contention) return Level::Contention;
        low |= d == Level::Low; high |= d == Level::High;
    }
    if (low && high) return Level::Contention;
    if (low) return Level::Low;
    if (high || pullup) return Level::High;
    return Level::Floating;
}
Esp8266Board::Esp8266Board() : Board("esp8266") { reset(); }
void Esp8266Board::reset() {
    pins_.clear();
    for (int p : nodeMcuGpio) pins_.emplace(p, Pin{});
    pins_.emplace(1, Pin{}); pins_.emplace(3, Pin{});
    adcVoltage_ = 0;
}
const Pin& Esp8266Board::pin(int gpio) const {
    auto p = pins_.find(gpio);
    if (p == pins_.end()) throw std::out_of_range("GPIO unavailable (6..11 reserved for flash)");
    return p->second;
}
void Esp8266Board::pinMode(int gpio, PinMode mode) {
    (void)pin(gpio);
    if (gpio == 16 && mode == PinMode::InputPullup) throw std::invalid_argument("GPIO16 has no INPUT_PULLUP");
    pins_.at(gpio).mode = mode;
}
void Esp8266Board::digitalWrite(int gpio, bool high) {
    (void)pin(gpio);
    if (pins_.at(gpio).mode != PinMode::Output) throw std::logic_error("digitalWrite requires OUTPUT in this model");
    pins_.at(gpio).output = high;
}
Level Esp8266Board::digitalRead(int gpio) const {
    const auto& p = pin(gpio);
    const auto own = p.mode == PinMode::Output ? (p.output ? Level::High : Level::Low) : Level::Floating;
    return DigitalNet::resolve({own, p.external}, p.mode == PinMode::InputPullup);
}
void Esp8266Board::driveInput(int gpio, Level level) { (void)pin(gpio); pins_.at(gpio).external = level; }
void Esp8266Board::setAdcVoltage(double volts) {
    if (!std::isfinite(volts) || volts < 0 || volts > 1) throw std::invalid_argument("A0 voltage outside 0..1 V");
    adcVoltage_ = volts;
}
int Esp8266Board::analogRead() const { return static_cast<int>(std::lround(adcVoltage_ * 1023)); }
Led::Led(std::string id, double resistance, double forwardVoltage) : Component(std::move(id)), resistance_(resistance), forwardVoltage_(forwardVoltage) {
    if (!std::isfinite(resistance) || resistance <= 0 || !std::isfinite(forwardVoltage) || forwardVoltage < 0) throw std::invalid_argument("Invalid LED parameters");
}
void Led::apply(double anodeVoltage, double cathodeVoltage) {
    if (!std::isfinite(anodeVoltage) || !std::isfinite(cathodeVoltage)) throw std::invalid_argument("Invalid voltage");
    current_ = std::max(0.0, (anodeVoltage - cathodeVoltage - forwardVoltage_) / resistance_);
}
double Led::brightness() const { return std::clamp(current_ / 0.02, 0.0, 1.0); }
Frame::Frame(int w, int h) : width(w), height(h) {
    if (w <= 0 || h <= 0 || w > 1024 || h > 1024) throw std::invalid_argument("Invalid frame size");
    pixels.resize(static_cast<std::size_t>(w) * h);
}
void Frame::setPixel(int x, int y, bool on) { if (x >= 0 && y >= 0 && x < width && y < height) pixels[static_cast<std::size_t>(y) * width + x] = on; }
bool Frame::pixel(int x, int y) const { return x >= 0 && y >= 0 && x < width && y < height && pixels.at(static_cast<std::size_t>(y) * width + x) != 0; }
OledDisplay::OledDisplay() : Display("ssd1306") {}
void OledDisplay::reset() { visible_ = Frame{}; revision_ = 0; }
void OledDisplay::present(const Frame& f) {
    if (f.width != 128 || f.height != 64 || f.pixels.size() != 8192) throw std::invalid_argument("SSD1306 requires a 128x64 frame");
    visible_ = f; ++revision_;
}
Microseconds OledDisplay::transferDuration(std::size_t bytes, unsigned hz) {
    if (!hz || hz > 400000 || bytes > 1048576) throw std::invalid_argument("Unsupported I2C transfer");
    // 9 clocks per byte including ACK; caller includes address/control bytes.
    return (static_cast<Microseconds>(bytes) * 9 * 1000000 + hz - 1) / hz;
}
void Simulator::schedule(Microseconds at, std::function<void()> action) {
    if (at < now_ || !action) throw std::invalid_argument("Cannot schedule in the past or an empty callback");
    if (sequence_ == std::numeric_limits<std::uint64_t>::max()) throw std::overflow_error("Event sequence overflow");
    events_.push({at, sequence_++, std::move(action)});
}
void Simulator::runUntil(Microseconds until, std::size_t budget) {
    if (until < now_) throw std::invalid_argument("Time cannot run backwards");
    std::size_t executed = 0;
    while (!events_.empty() && events_.top().at <= until) {
        if (executed++ >= budget) throw std::runtime_error("Event budget exceeded: possible zero-time loop");
        auto event = events_.top(); events_.pop(); now_ = event.at; event.action();
    }
    now_ = until;
}
void Simulator::reset() { events_ = {}; now_ = 0; sequence_ = 0; }
void Animation::addFrame(Frame f, Microseconds duration) {
    if (!duration || f.width != 128 || f.height != 64 || f.pixels.size() != 8192) throw std::invalid_argument("Invalid animation frame/duration");
    entries_.push_back({std::move(f), duration});
}
void Animation::enqueue(Simulator& sim, OledDisplay& display, Microseconds start) const {
    auto at = start;
    if (start < sim.now()) throw std::invalid_argument("Animation starts in the past");
    for (const auto& e : entries_) {
        if (e.duration > std::numeric_limits<Microseconds>::max() - at) throw std::overflow_error("Animation duration overflow");
        at += e.duration;
    }
    at = start;
    for (const auto& e : entries_) {
        sim.schedule(at, [&display, f = e.frame] { display.present(f); });
        at += e.duration;
    }
}
}
