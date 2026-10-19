#pragma once
#include "Arduino.h"
#include <vector>
// Pin-level compatibility layer: real GPIO writes drive the wired motor model.
class Stepper {
    int steps_, phase_ = 0;
    unsigned interval_ = 60000;
    std::vector<int> pins_;

  public:
    Stepper(int steps, int a, int b) : steps_(steps), pins_{a, b} {
        initialize();
    }
    Stepper(int steps, int a, int b, int c, int d) : steps_(steps), pins_{a, b, c, d} {
        initialize();
    }
    void initialize() {
        if (steps_ <= 0)
            throw std::invalid_argument("Stepper steps per revolution must be positive");
        for (int pin : pins_)
            pinMode(pin, OUTPUT);
    }
    void setSpeed(long rpm) {
        if (rpm <= 0 || rpm > 60000)
            throw std::invalid_argument("Stepper RPM outside supported range");
        interval_ = std::max(1UL, 60000000UL / static_cast<unsigned long>(steps_) / rpm);
    }
    void step(int count) {
        const int direction = count < 0 ? -1 : 1;
        auto remaining = count < 0 ? -static_cast<long long>(count) : static_cast<long long>(count);
        while (remaining--) {
            phase_ = (phase_ + direction + 4) % 4;
            const unsigned pattern = pins_.size() == 2 ? unsigned(phase_ ^ (phase_ >> 1))
                                                       : std::array<unsigned, 4>{5, 6, 10, 9}[phase_];
            for (std::size_t i = 0; i < pins_.size(); ++i)
                digitalWrite(pins_[i], (pattern >> i) & 1);
            delayMicroseconds(interval_);
        }
    }
    int version() const {
        return 5;
    }
};
