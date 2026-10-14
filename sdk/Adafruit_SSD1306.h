#pragma once
#include "Wire.h"
#include <vector>
constexpr int SSD1306_SWITCHCAPVCC = 2;
constexpr int SSD1306_BLACK = 0, SSD1306_WHITE = 1, SSD1306_INVERSE = 2;
class Adafruit_SSD1306 {
public:
    Adafruit_SSD1306(int w, int h, TwoWire* wire = &Wire, int = -1) : wire_(wire) {
        if (w != 128 || h != 64 || !wire) throw std::invalid_argument("Only SSD1306 128x64 supported");
    }
    bool begin(int, unsigned char address = 0x3c, bool = true, bool periphBegin = true) {
        if (address != 0x3c) return false;
        if (periphBegin && !wire_->ready()) wire_->begin();
        begun_ = wire_->ready(); return begun_;
    }
    void clearDisplay() { pixels_.fill(0); }
    void drawPixel(int x, int y, unsigned color) {
        if (x < 0 || x >= 128 || y < 0 || y >= 64) return;
        auto& b = pixels_.at(static_cast<std::size_t>(x + (y/8)*128));
        const auto mask = static_cast<unsigned char>(1u << (y%8));
        if (color == SSD1306_INVERSE) b ^= mask;
        else if (color) b |= mask;
        else b &= static_cast<unsigned char>(~mask);
    }
    void fillRect(int x, int y, int w, int h, unsigned color) {
        // Clip before iterating, including potentially overflowing sketch coordinates.
        const auto right = std::min<std::int64_t>(128, static_cast<std::int64_t>(x)+w);
        const auto bottom = std::min<std::int64_t>(64, static_cast<std::int64_t>(y)+h);
        for (int yy=std::max(y,0); yy<bottom; ++yy)
            for (int xx=std::max(x,0); xx<right; ++xx) drawPixel(xx,yy,color);
    }
    // Arduino/GFX bitmaps: row-major, MSB first, each row padded to full bytes.
    void drawBitmap(int x, int y, const unsigned char* bitmap, int w, int h, unsigned color) {
        if (!bitmap || w <= 0 || h <= 0 || w > 1024 || h > 1024) throw std::invalid_argument("Invalid bitmap");
        for (int yy=0; yy<h; ++yy) for (int xx=0; xx<w; ++xx)
            if (pgm_read_byte(bitmap + yy*((w+7)/8)+xx/8) & (0x80u >> (xx%8))) {
                const auto dx = static_cast<std::int64_t>(x)+xx, dy = static_cast<std::int64_t>(y)+yy;
                if (dx>=0 && dx<128 && dy>=0 && dy<64) drawPixel(static_cast<int>(dx),static_cast<int>(dy),color);
            }
    }
    unsigned char* getBuffer() { return pixels_.data(); }
    void display() {
        if (!begun_) throw std::logic_error("Call display.begin first");
        thinker::advance((1026ull*9*1000000 + wire_->clock()-1)/wire_->clock());
        constexpr char hex[]="0123456789abcdef";
        std::string line="FRAME "; line.reserve(2054);
        for (auto b : pixels_) { line += hex[b>>4]; line += hex[b&15]; }
        std::cout << line << std::endl;
    }
private:
    TwoWire* wire_;
    bool begun_ = false;
    std::array<unsigned char,1024> pixels_{};
};
