#pragma once
#include "Arduino.h"
// MH-Z19B UART read command, using the provided Stream and the sensor's checksum.
class MHZ19 {
    Stream *serial_ = nullptr;
    unsigned timeout_ = 1000;
    int lastTemperature_ = 0;
    bool valid_ = false;

  public:
    void begin(Stream &serial) {
        serial_ = &serial;
        valid_ = false;
    }
    void setTimeout(unsigned timeout) {
        if (!timeout || timeout > 10000)
            throw std::invalid_argument("MH-Z19 timeout outside 1..10000 ms");
        timeout_ = timeout;
    }
    int getCO2(bool = false) {
        if (!serial_)
            throw std::logic_error("Call MHZ19.begin first");
        valid_ = false;
        while (serial_->available())
            serial_->read();
        const uint8_t request[] = {0xff, 1, 0x86, 0, 0, 0, 0, 0, 0x79};
        serial_->write(request, sizeof(request));
        std::array<uint8_t, 9> response{};
        std::size_t count = 0;
        const auto start = uint32_t(millis());
        while (count < response.size() && uint32_t(millis()) - start < timeout_) {
            if (serial_->available()) {
                const int value = serial_->read();
                if (value < 0)
                    continue;
                if (!count && value != 0xff)
                    continue;
                response[count++] = uint8_t(value);
            } else
                delay(1);
        }
        unsigned sum = 0;
        for (int i = 1; i < 8; ++i)
            sum += response[i];
        if (count != 9 || response[1] != 0x86 || response[8] != uint8_t(0x100 - sum))
            return -1;
        valid_ = true;
        lastTemperature_ = int(response[4]) - 40;
        return int(response[2]) * 256 + response[3];
    }
    int getTemperature() const {
        return valid_ ? lastTemperature_ : 0;
    }
    bool isValid() const {
        return valid_;
    }
    void autoCalibration(bool = true) {
        throw std::logic_error(
            "MH-Z19 chemical calibration is not modeled; set simulated CO2 in the inspector");
    }
    void calibrate() {
        throw std::logic_error(
            "MH-Z19 chemical calibration is not modeled; set simulated CO2 in the inspector");
    }
};
