#include "Adafruit_ILI9341.h"
#include "Adafruit_ST7735.h"
#include <stdexcept>
static void check(bool value) {
    if (!value)
        throw std::runtime_error("TFT SDK contract failed");
}
int main() {
    thinker::standalone = true;
    Adafruit_ILI9341 tft(15, 4, 5);
    tft.begin();
    const auto before = thinker::clockUs;
    tft.fillScreen(ILI9341_RED);
    check(thinker::clockUs - before >= 240 * 320 * 2); // RGB565 at the default 8 MHz.
    tft.setRotation(1);
    check(tft.width() == 320 && tft.height() == 240);
    tft.setCursor(4, 4);
    tft.setTextColor(ILI9341_WHITE, ILI9341_RED);
    tft.setTextSize(2);
    tft.print("ThinkCad");
    tft.drawLine(1, 1, 30, 30, ILI9341_GREEN);
    tft.drawCircle(50, 50, 10, ILI9341_BLUE);
    const uint16_t bitmap[] = {ILI9341_RED, ILI9341_GREEN, ILI9341_BLUE, ILI9341_WHITE};
    tft.drawRGBBitmap(0, 0, bitmap, 2, 2);
    tft.setScrollMargins(0, 0);
    tft.scrollTo(4);
    tft.invertDisplay(true);
    Adafruit_ST7735 small(15, 4, 5);
    small.initR(INITR_BLACKTAB);
    small.setRotation(1);
    check(small.width() == 160 && small.height() == 128);
    small.fillRect(0, 0, 20, 20, ST77XX_YELLOW);
}
