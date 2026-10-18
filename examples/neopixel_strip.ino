#include <Adafruit_NeoPixel.h>
Adafruit_NeoPixel strip(8, 6, NEO_GRB + NEO_KHZ800);
uint16_t hue = 0;
void setup() {
  strip.begin();
  strip.setBrightness(32);
  strip.fill(strip.Color(255, 0, 0));
  strip.show();
  Serial.println("NEOPIXEL_READY");
}
void loop() {
  strip.rainbow(hue, 1, 255, 255, true);
  strip.show();
  hue += 256;
  delay(20);
}
