#pragma once
#include "ThinkCad_TFT.h"
constexpr uint8_t INITR_GREENTAB = 0, INITR_REDTAB = 1, INITR_BLACKTAB = 2, INITR_144GREENTAB = 1,
                  INITR_MINI160x80 = 4;
constexpr uint16_t ST77XX_BLACK = 0, ST77XX_WHITE = 0xffff, ST77XX_RED = 0xf800, ST77XX_GREEN = 0x07e0,
                   ST77XX_BLUE = 0x001f, ST77XX_CYAN = 0x07ff, ST77XX_MAGENTA = 0xf81f,
                   ST77XX_YELLOW = 0xffe0, ST77XX_ORANGE = 0xfc00;
constexpr uint16_t ST7735_BLACK = ST77XX_BLACK, ST7735_WHITE = ST77XX_WHITE, ST7735_RED = ST77XX_RED,
                   ST7735_GREEN = ST77XX_GREEN, ST7735_BLUE = ST77XX_BLUE, ST7735_CYAN = ST77XX_CYAN,
                   ST7735_MAGENTA = ST77XX_MAGENTA, ST7735_YELLOW = ST77XX_YELLOW;
class Adafruit_ST7735 : public ThinkCad_TFT {
  public:
    Adafruit_ST7735(int8_t cs, int8_t dc, int8_t reset = -1) : ThinkCad_TFT(128, 160, cs, dc, reset) {}
    Adafruit_ST7735(int8_t cs, int8_t dc, int8_t mosi, int8_t sck, int8_t reset, int8_t miso = -1)
        : ThinkCad_TFT(128, 160, cs, dc, mosi, sck, reset, miso) {}
    Adafruit_ST7735(SPIClass *spi, int8_t cs, int8_t dc, int8_t reset = -1)
        : ThinkCad_TFT(128, 160, cs, dc, reset, spi) {}
    void initR(uint8_t options = INITR_GREENTAB) {
        if (options > 2)
            throw std::invalid_argument("This ST7735 model supports 128x160 panels only");
        initialize();
    }
    void initB() {
        initialize();
    }
};
