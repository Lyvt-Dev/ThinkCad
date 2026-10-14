#pragma once
#include "Arduino.h"
// High-level simulator adapter; not an implementation of arbitrary I2C devices.
class TwoWire {
public:
    void begin(int sda = D2, int scl = D1) {
        thinker::validatePin(sda); thinker::validatePin(scl);
        if (sda == scl) throw std::invalid_argument("SDA and SCL must differ");
        sda_ = sda; scl_ = scl; initialized_ = true;
        std::cout << "I2C " << sda << ' ' << scl << std::endl;
    }
    void setClock(unsigned long hz) {
        if (hz == 0 || hz > 400000) throw std::invalid_argument("I2C supports up to 400 kHz");
        hz_ = hz;
    }
    unsigned long clock() const { return hz_; }
    bool ready() const { return initialized_; }
private:
    int sda_ = D2, scl_ = D1;
    unsigned long hz_ = 100000;
    bool initialized_ = false;
};
inline TwoWire Wire;
