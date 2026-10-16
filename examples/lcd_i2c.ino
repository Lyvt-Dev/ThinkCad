#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27,20,4);
byte heart[8]={0,10,31,31,14,4,0,0};
void setup(){lcd.init();lcd.backlight();lcd.createChar(0,heart);lcd.setCursor(0,0);lcd.print("ThinkCad 20x4");lcd.setCursor(0,1);lcd.print("LiquidCrystal_I2C");lcd.setCursor(0,2);lcd.write(byte(0));}
void loop(){lcd.setCursor(0,3);lcd.print("Sekunden: ");lcd.print(millis()/1000);delay(200);}
