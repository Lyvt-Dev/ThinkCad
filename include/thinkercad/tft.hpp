#pragma once
#include "circuit.hpp"
namespace thinkercad {
// A command-level SPI display model, not a pixel-clock or analogue panel emulator.
class TftController {
  public:
    TftController() = default;
    TftController(int width, int height, bool ili9341);
    void reset();
    std::uint8_t transfer(bool data, std::uint8_t value, Microseconds now);
    std::shared_ptr<const ColorFrame> frame(Microseconds now, double brightness);
    const std::string &error() const {
        return error_;
    }
    int width() const {
        return width_;
    }
    int height() const {
        return height_;
    }

  private:
    int width_ = 0, height_ = 0;
    bool ili_ = false, sleep_ = true, on_ = false, invert_ = false, format_ = false;
    std::uint8_t command_ = 0, madctl_ = 0x08, high_ = 0;
    bool half_ = false, xValid_ = true, yValid_ = true;
    std::vector<std::uint16_t> pixels_;
    std::vector<std::uint8_t> arguments_;
    unsigned x0_ = 0, y0_ = 0, x1_ = 0, y1_ = 0, x_ = 0, y_ = 0, read_ = 0, top_ = 0, bottom_ = 0,
             scroll_ = 0;
    Microseconds ready_ = 0;
    std::uint64_t revision_ = 0, cachedRevision_ = ~std::uint64_t(0);
    std::shared_ptr<const ColorFrame> cached_;
    std::string error_;
    void parameter();
    void next();
};
} // namespace thinkercad
