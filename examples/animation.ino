#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
Adafruit_SSD1306 display(128, 64, &Wire, -1);
// Two 8x8 frames; arbitrary arrays generated from monochrome images also work.
const uint8_t frames[][8] PROGMEM = {
    {0x18,0x3c,0x7e,0xdb,0xff,0x24,0x5a,0xa5},
    {0x18,0x3c,0x7e,0xdb,0xff,0x5a,0xa5,0x24}
};
unsigned frameIndex = 0;
void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    Wire.begin(D2, D1);
    Wire.setClock(400000);
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3c)) {
        Serial.println("OLED missing");
        for (;;) delay(1000);
    }
}
void loop() {
    display.clearDisplay();
    const int x = static_cast<int>((millis()/8)%120);
    display.drawBitmap(x, 28, frames[frameIndex%2], 8, 8, SSD1306_WHITE);
    display.fillRect(8, 52, 112, 2, SSD1306_WHITE);
    display.display();
    digitalWrite(LED_BUILTIN, frameIndex%2 ? LOW : HIGH);
    ++frameIndex;
    delay(77); // + ~23.1ms transfer at 400kHz gives ~10 frames/s.
}
