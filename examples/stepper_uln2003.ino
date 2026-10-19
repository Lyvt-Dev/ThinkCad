#include <Stepper.h>
// Constructor order follows the physical coil sequence: IN1, IN3, IN2, IN4.
Stepper motor(2048,2,4,3,5);
void setup(){Serial.begin(115200);motor.setSpeed(5);motor.step(1);Serial.println("Coils aligned; relative position zero");}
void loop(){motor.step(32);Serial.println("relative steps=32");delay(200);motor.step(-32);Serial.println("relative steps=0");delay(200);}
