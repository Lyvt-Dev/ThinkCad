#include <Servo.h>
Servo servo;
void setup(){servo.attach(9);}
void loop(){for(int angle=15;angle<=165;angle+=5){servo.write(angle);Serial.print("Winkel:");Serial.println(angle);delay(40);}for(int angle=165;angle>=15;angle-=5){servo.write(angle);delay(40);}}
