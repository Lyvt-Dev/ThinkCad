#pragma once
#include "Arduino.h"
#include "Adafruit_GFX.h"
#include "SPI.h"
#include <vector>
// Host graphics adapter. Uses actual wired SPI commands and simulated transfer time.
class ThinkCad_TFT : public Adafruit_GFX {
    SPIClass software_;
    SPIClass *spi_;
    int cs_, dc_, reset_, mosi_ = -1, sck_ = -1, miso_ = -1, depth_ = 0;
    unsigned frequency_ = 8000000;
    bool begun_ = false, dirty_ = false;
    int left_ = 0, top_ = 0, right_ = 0, bottom_ = 0;
    std::vector<uint16_t> pixels_;
    void select(bool active) {
        if (cs_ >= 0)
            digitalWrite(cs_, active ? LOW : HIGH);
    }
    void command(uint8_t value, const std::vector<uint8_t> &data = {}) {
        spi_->beginTransaction(SPISettings(frequency_, MSBFIRST, SPI_MODE0));
        select(true);
        digitalWrite(dc_, LOW);
        spi_->writeBytes(&value, 1);
        if (!data.empty()) {
            digitalWrite(dc_, HIGH);
            spi_->writeBytes(data.data(), data.size());
        }
        select(false);
        spi_->endTransaction();
    }
    bool map(int &x, int &y) const {
        if (x < 0 || y < 0 || x >= width() || y >= height())
            return false;
        int t;
        switch (getRotation()) {
        case 1:
            t = x;
            x = WIDTH - y - 1;
            y = t;
            break;
        case 2:
            x = WIDTH - x - 1;
            y = HEIGHT - y - 1;
            break;
        case 3:
            t = x;
            x = y;
            y = HEIGHT - t - 1;
            break;
        }
        return true;
    }
    void mark(int x, int y) {
        if (!dirty_) {
            left_ = right_ = x;
            top_ = bottom_ = y;
            dirty_ = true;
        } else {
            left_ = std::min(left_, x);
            right_ = std::max(right_, x);
            top_ = std::min(top_, y);
            bottom_ = std::max(bottom_, y);
        }
    }
    void flush() {
        if (!dirty_ || !begun_)
            return;
        command(0x2a, {uint8_t(left_ >> 8), uint8_t(left_), uint8_t(right_ >> 8), uint8_t(right_)});
        command(0x2b, {uint8_t(top_ >> 8), uint8_t(top_), uint8_t(bottom_ >> 8), uint8_t(bottom_)});
        const uint8_t write = 0x2c;
        spi_->beginTransaction(SPISettings(frequency_));
        select(true);
        digitalWrite(dc_, LOW);
        spi_->writeBytes(&write, 1);
        digitalWrite(dc_, HIGH);
        std::array<uint8_t, 1024> bytes{};
        std::size_t count = 0;
        for (int y = top_; y <= bottom_; ++y)
            for (int x = left_; x <= right_; ++x) {
                const auto color = pixels_[y * WIDTH + x];
                bytes[count++] = uint8_t(color >> 8);
                bytes[count++] = uint8_t(color);
                if (count == bytes.size()) {
                    spi_->writeBytes(bytes.data(), count);
                    count = 0;
                }
            }
        if (count)
            spi_->writeBytes(bytes.data(), count);
        select(false);
        spi_->endTransaction();
        dirty_ = false;
    }

  protected:
    ThinkCad_TFT(int w, int h, int cs, int dc, int reset, SPIClass *spi = &SPI)
        : Adafruit_GFX(w, h), spi_(spi), cs_(cs), dc_(dc), reset_(reset), pixels_(w * h, 0) {
        if (!spi_)
            throw std::invalid_argument("TFT SPI instance is null");
    }
    ThinkCad_TFT(int w, int h, int cs, int dc, int mosi, int sck, int reset, int miso)
        : ThinkCad_TFT(w, h, cs, dc, reset, &software_) {
        mosi_ = mosi;
        sck_ = sck;
        miso_ = miso;
    }
    void initialize(uint32_t frequency = 0) {
        frequency_ = frequency ? frequency : 8000000;
        if (frequency_ > 20000000)
            throw std::invalid_argument("TFT SPI clock exceeds 20 MHz model limit");
        if (cs_ >= 0)
            pinMode(cs_, OUTPUT);
        pinMode(dc_, OUTPUT);
        select(false);
        digitalWrite(dc_, HIGH);
        if (sck_ >= 0)
            spi_->begin(sck_, miso_, mosi_, cs_);
        else if (!spi_->ready())
            spi_->begin();
        if (reset_ >= 0) {
            pinMode(reset_, OUTPUT);
            digitalWrite(reset_, LOW);
            delay(10);
            digitalWrite(reset_, HIGH);
            delay(120);
        }
        command(0x01);
        delay(150);
        command(0x11);
        delay(120);
        command(0x3a, {0x55});
        command(0x36, {0x08});
        command(0x29);
        begun_ = true;
    }

  public:
    void startWrite() override {
        ++depth_;
    }
    void endWrite() override {
        if (depth_ > 0)
            --depth_;
        if (!depth_)
            flush();
    }
    void drawPixel(int16_t x, int16_t y, uint16_t color) override {
        int px = x, py = y;
        if (!map(px, py))
            return;
        pixels_[py * WIDTH + px] = color;
        mark(px, py);
        if (!depth_)
            flush();
    }
    void writePixel(int16_t x, int16_t y, uint16_t color) override {
        drawPixel(x, y, color);
    }
    void writeFillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) override {
        if (w <= 0 || h <= 0)
            return;
        const int x0 = std::max(0, int(x)), y0 = std::max(0, int(y)), x1 = std::min(int(width()), int(x) + w),
                  y1 = std::min(int(height()), int(y) + h);
        ++depth_;
        for (int row = y0; row < y1; ++row)
            for (int col = x0; col < x1; ++col)
                drawPixel(col, row, color);
        --depth_;
        if (!depth_)
            flush();
    }
    void writeFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) override {
        writeFillRect(x, y, w, 1, color);
    }
    void writeFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) override {
        writeFillRect(x, y, 1, h, color);
    }
    void invertDisplay(bool value) override {
        command(value ? 0x21 : 0x20);
    }
    void enableDisplay(bool value) {
        command(value ? 0x29 : 0x28);
    }
    void enableSleep(bool value) {
        command(value ? 0x10 : 0x11);
        if (!value)
            delay(120);
    }
    void setScrollMargins(uint16_t top, uint16_t bottom) {
        if (unsigned(top) + bottom >= unsigned(HEIGHT))
            throw std::invalid_argument("Invalid TFT scroll margins");
        const auto area = HEIGHT - top - bottom;
        command(0x33, {uint8_t(top >> 8), uint8_t(top), uint8_t(area >> 8), uint8_t(area),
                       uint8_t(bottom >> 8), uint8_t(bottom)});
    }
    void scrollTo(uint16_t y) {
        command(0x37, {uint8_t(y >> 8), uint8_t(y)});
    }
    static uint16_t color565(uint8_t r, uint8_t g, uint8_t b) {
        return uint16_t((r & 0xf8) << 8) | uint16_t((g & 0xfc) << 3) | uint16_t(b >> 3);
    }
    void setSPISpeed(uint32_t frequency) {
        if (!frequency || frequency > 20000000)
            throw std::invalid_argument("Invalid TFT SPI frequency");
        frequency_ = frequency;
    }
    ThinkCad_TFT(const ThinkCad_TFT &) = delete;
    ThinkCad_TFT &operator=(const ThinkCad_TFT &) = delete;
};
