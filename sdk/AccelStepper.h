#pragma once
#include "Arduino.h"
// Supported subset, implemented through GPIO and simulated time. No mechanical model.
class AccelStepper {
  public:
    enum {
        FUNCTION = 0,
        DRIVER = 1,
        FULL2WIRE = 2,
        FULL3WIRE = 3,
        FULL4WIRE = 4,
        HALF3WIRE = 6,
        HALF4WIRE = 8
    };
    explicit AccelStepper(uint8_t interface = FULL4WIRE, uint8_t pin1 = 2, uint8_t pin2 = 3, uint8_t pin3 = 4,
                          uint8_t pin4 = 5, bool enable = true)
        : interface_(interface), pins_{pin1, pin2, pin3, pin4} {
        if (interface != DRIVER && interface != FULL4WIRE && interface != HALF4WIRE && interface != FULL2WIRE)
            throw std::invalid_argument("AccelStepper supports DRIVER, FULL2WIRE, FULL4WIRE and HALF4WIRE");
        if (enable)
            enableOutputs();
    }
    void setMaxSpeed(float value) {
        positive(value);
        maximum_ = value;
        if (std::abs(speed_) > maximum_)
            setSpeed(std::copysign(maximum_, speed_));
    }
    float maxSpeed() const {
        return maximum_;
    }
    void setAcceleration(float value) {
        positive(value);
        acceleration_ = value;
    }
    float acceleration() const {
        return acceleration_;
    }
    void setSpeed(float value) {
        if (!std::isfinite(value))
            throw std::invalid_argument("Nonfinite stepper speed");
        speed_ = std::clamp(value, -maximum_, maximum_);
    }
    float speed() const {
        return speed_;
    }
    void moveTo(long value) {
        target_ = value;
#ifdef THINKERCAD_SIMULATION
        if (interface_ == DRIVER)
            thinker::request("STEPPER_TARGET " + std::to_string(pins_[0]) + " " + std::to_string(value));
#endif
    }
    void move(long delta) {
        const auto next = static_cast<long long>(position_) + delta;
        if (next < std::numeric_limits<long>::min() || next > std::numeric_limits<long>::max())
            throw std::out_of_range("Stepper target overflow");
        moveTo(static_cast<long>(next));
    }
    long currentPosition() const {
        return position_;
    }
    long targetPosition() const {
        return target_;
    }
    long distanceToGo() const {
        const auto d = static_cast<long long>(target_) - position_;
        return static_cast<long>(std::clamp(d, static_cast<long long>(std::numeric_limits<long>::min()),
                                            static_cast<long long>(std::numeric_limits<long>::max())));
    }
    void setCurrentPosition(long value) {
        position_ = value;
        moveTo(value);
        speed_ = 0;
    }
    bool isRunning() const {
        return position_ != target_ || speed_ != 0;
    }
    bool runSpeed() {
        if (!enabled_ || speed_ == 0)
            return false;
        const auto now = static_cast<uint32_t>(micros());
        const auto interval =
            static_cast<uint32_t>(std::clamp(1000000. / std::abs(speed_), 1., double(UINT32_MAX)));
        if (static_cast<uint32_t>(now - last_) < interval)
            return false;
        const int direction = speed_ > 0 ? 1 : -1;
        if ((direction > 0 && position_ == std::numeric_limits<long>::max()) ||
            (direction < 0 && position_ == std::numeric_limits<long>::min()))
            throw std::out_of_range("Stepper position overflow");
        position_ += direction;
        output(direction);
        last_ = now;
        return true;
    }
    bool run() {
        const auto distance = distanceToGo();
        if (!distance) {
            speed_ = 0;
            return false;
        }
        const float desired = std::copysign(
            std::min(maximum_, std::sqrt(2.f * acceleration_ * std::abs(float(distance)))), float(distance));
        if (speed_ == 0)
            speed_ = std::copysign(std::min(maximum_, std::sqrt(2.f * acceleration_)), desired);
        if (runSpeed()) {
            const float increment = acceleration_ / std::max(1.f, std::abs(speed_));
            speed_ += std::clamp(desired - speed_, -increment, increment);
            if (position_ == target_)
                speed_ = 0;
        }
        return isRunning();
    }
    bool runSpeedToPosition() {
        if (position_ == target_)
            return false;
        if (speed_ != 0)
            speed_ = std::copysign(std::abs(speed_), float(distanceToGo()));
        return runSpeed();
    }
    void runToPosition() {
        while (run())
            delayMicroseconds(50);
    }
    void runToNewPosition(long value) {
        moveTo(value);
        runToPosition();
    }
    void stop() {
        const auto steps = static_cast<long>(std::ceil(double(speed_) * speed_ / (2 * acceleration_)));
        move(speed_ >= 0 ? steps : -steps);
    }
    void setEnablePin(uint8_t pin = 0xff) {
        enablePin_ = pin;
        if (pin != 0xff) {
            pinMode(pin, OUTPUT);
            digitalWrite(pin, enabled_ ? !enableInvert_ : enableInvert_);
        }
    }
    void setPinsInverted(bool direction = false, bool step = false, bool enable = false) {
        directionInvert_ = direction;
        stepInvert_ = step;
        enableInvert_ = enable;
    }
    void setMinPulseWidth(unsigned int width) {
        if (width > 10000)
            throw std::invalid_argument("Stepper pulse width exceeds 10000 us");
        pulse_ = std::max(1u, width);
    }
    void enableOutputs() {
        for (int i = 0; i < (interface_ == DRIVER || interface_ == FULL2WIRE ? 2 : 4); ++i)
            pinMode(pins_[i], OUTPUT);
        enabled_ = true;
        if (enablePin_ != 0xff)
            digitalWrite(enablePin_, !enableInvert_);
    }
    void disableOutputs() {
        for (int i = 0; i < (interface_ == DRIVER || interface_ == FULL2WIRE ? 2 : 4); ++i)
            digitalWrite(pins_[i], LOW);
        enabled_ = false;
        if (enablePin_ != 0xff)
            digitalWrite(enablePin_, enableInvert_);
    }

  private:
    uint8_t interface_, pins_[4], enablePin_ = 0xff;
    long position_ = 0, target_ = 0;
    float speed_ = 0, maximum_ = 1, acceleration_ = 1;
    uint32_t last_ = 0;
    unsigned pulse_ = 1;
    bool enabled_ = false, directionInvert_ = false, stepInvert_ = false, enableInvert_ = false;
    static void positive(float value) {
        if (!std::isfinite(value) || value <= 0 || value > 1000000)
            throw std::invalid_argument("Stepper speed/acceleration outside supported range");
    }
    void output(int direction) {
        if (interface_ == DRIVER) {
            digitalWrite(pins_[1], (direction > 0) ^ directionInvert_);
            digitalWrite(pins_[0], !stepInvert_);
            delayMicroseconds(pulse_);
            digitalWrite(pins_[0], stepInvert_);
            return;
        }
        const int phase = int((position_ % 8 + 8) % 8);
        const unsigned pattern = interface_ == HALF4WIRE
                                     ? std::array<unsigned, 8>{1, 5, 4, 6, 2, 10, 8, 9}[phase]
                                 : interface_ == FULL2WIRE ? unsigned((phase % 4) ^ ((phase % 4) >> 1))
                                                           : std::array<unsigned, 4>{5, 6, 10, 9}[phase % 4];
        for (int i = 0; i < (interface_ == FULL2WIRE ? 2 : 4); ++i)
            digitalWrite(pins_[i], (pattern >> i) & 1);
    }
};
