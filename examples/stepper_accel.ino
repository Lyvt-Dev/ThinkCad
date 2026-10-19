#include <AccelStepper.h>
AccelStepper motor(AccelStepper::DRIVER,2,3);
void setup(){Serial.begin(115200);motor.setPinsInverted(false,false,true);motor.setEnablePin(4);motor.setMaxSpeed(200);motor.setAcceleration(1000);motor.moveTo(20);}
void loop(){if(!motor.run()){Serial.print("position=");Serial.println(motor.currentPosition());delay(200);motor.moveTo(motor.currentPosition()==0?20:0);}delayMicroseconds(500);}
