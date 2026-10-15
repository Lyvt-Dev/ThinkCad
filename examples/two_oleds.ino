#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <pgmspace.h>
// Both displays share SDA D2 and SCL D1; addresses are 0x3C and 0x3D.
Adafruit_SSD1306 leftDisplay(128, 64, &Wire, -1);
Adafruit_SSD1306 rightDisplay(128, 64, &Wire, -1);
const uint8_t frames[][8] PROGMEM = {
    {0x18,0x3c,0x7e,0xdb,0xff,0x24,0x5a,0xa5},
    {0x18,0x3c,0x7e,0xdb,0xff,0x5a,0xa5,0x24}
};
void setup() {
    Wire.begin(D2, D1); Wire.setClock(400000);
    if (!leftDisplay.begin(SSD1306_SWITCHCAPVCC, 0x3c) ||
        !rightDisplay.begin(SSD1306_SWITCHCAPVCC, 0x3d)) {
        Serial.println(F("Check display power, wiring and addresses"));
        for (;;) delay(1000);
    }
}
void loop() {
    const int position = (millis() / 12) % 120;
    leftDisplay.clearDisplay(); rightDisplay.clearDisplay();
    leftDisplay.drawBitmap(position, 28, frames[(millis()/150)%2], 8, 8, SSD1306_WHITE);
    rightDisplay.drawBitmap(120-position, 28, frames[(millis()/150)%2], 8, 8, SSD1306_WHITE);
    leftDisplay.display(); rightDisplay.display();
    delay(54);
}
