#include <Stepper.h>
// AC and BC are tied to 5 V. ULN2003 outputs connect A1,B1,A2,B2.
Stepper motor(200,2,4,3,5);
void setup(){Serial.begin(115200);motor.setSpeed(30);motor.step(1);Serial.println("Coils aligned; relative position zero");}
void loop(){motor.step(50);Serial.println("relative angle=90");delay(300);motor.step(-50);Serial.println("relative angle=0");delay(300);}
