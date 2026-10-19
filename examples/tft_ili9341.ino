#include <Adafruit_ILI9341.h>
Adafruit_ILI9341 display(10,9,8);
void setup(){Serial.begin(115200);display.begin();display.setRotation(1);display.fillScreen(ILI9341_BLACK);display.setCursor(5,5);display.setTextColor(ILI9341_WHITE);display.setTextSize(2);display.print("ThinkCad");display.drawCircle(45,65,20,ILI9341_CYAN);Serial.println("TFT ready");}
void loop(){static bool on=false;on=!on;display.fillRect(80,50,30,30,on?ILI9341_RED:ILI9341_GREEN);delay(200);}
