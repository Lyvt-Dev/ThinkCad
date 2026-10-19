#include <Adafruit_ST7735.h>
Adafruit_ST7735 display(10,9,8);
void setup(){Serial.begin(115200);display.initR(INITR_BLACKTAB);display.setRotation(1);display.fillScreen(ST77XX_BLACK);display.setCursor(5,5);display.setTextColor(ST77XX_WHITE);display.setTextSize(2);display.print("ThinkCad");display.drawCircle(45,65,20,ST77XX_CYAN);Serial.println("TFT ready");}
void loop(){static bool on=false;on=!on;display.fillRect(80,50,30,30,on?ST77XX_RED:ST77XX_GREEN);delay(200);}
