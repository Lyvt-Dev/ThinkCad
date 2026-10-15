#include <Adafruit_SSD1306.h>
Adafruit_SSD1306 display(128, 32, &Wire, -1);
void setup() {
  Wire.begin(D2, D1);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3c)) return;
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(0, 8);
  display.println("ThinkCad");
  display.display();
}
void loop() { delay(100); }
