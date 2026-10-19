#pragma once
#include "ThinkCad_TFT.h"
constexpr uint16_t ILI9341_BLACK = 0x0000, ILI9341_NAVY = 0x000f, ILI9341_DARKGREEN = 0x03e0,
                   ILI9341_DARKCYAN = 0x03ef, ILI9341_MAROON = 0x7800, ILI9341_PURPLE = 0x780f,
                   ILI9341_OLIVE = 0x7be0, ILI9341_LIGHTGREY = 0xc618, ILI9341_DARKGREY = 0x7bef,
                   ILI9341_BLUE = 0x001f, ILI9341_GREEN = 0x07e0, ILI9341_CYAN = 0x07ff, ILI9341_RED = 0xf800,
                   ILI9341_MAGENTA = 0xf81f, ILI9341_YELLOW = 0xffe0, ILI9341_WHITE = 0xffff,
                   ILI9341_ORANGE = 0xfd20, ILI9341_GREENYELLOW = 0xafe5, ILI9341_PINK = 0xfc18;
class Adafruit_ILI9341 : public ThinkCad_TFT {
  public:
    Adafruit_ILI9341(int8_t cs, int8_t dc, int8_t reset = -1) : ThinkCad_TFT(240, 320, cs, dc, reset) {}
    Adafruit_ILI9341(int8_t cs, int8_t dc, int8_t mosi, int8_t sck, int8_t reset, int8_t miso = -1)
        : ThinkCad_TFT(240, 320, cs, dc, mosi, sck, reset, miso) {}
    Adafruit_ILI9341(SPIClass *spi, int8_t dc, int8_t cs = -1, int8_t reset = -1)
        : ThinkCad_TFT(240, 320, cs, dc, reset, spi) {}
    void begin(uint32_t frequency = 0) {
        initialize(frequency);
    }
};
