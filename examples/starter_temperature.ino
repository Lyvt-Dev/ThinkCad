#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
Adafruit_SSD1306 display(128, 64, &Wire, -1);
void setup() {
  Wire.begin(21, 22);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
}
void loop() {
  const double temperature = (analogRead(34) * 3.3 / 4095.0 - 0.5) * 100.0;
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("TMP36: "); display.print(temperature); display.println(" C");
  display.display();
  delay(250);
}
