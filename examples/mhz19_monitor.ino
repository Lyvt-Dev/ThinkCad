#include <MHZ19.h>
MHZ19 sensor;
void setup(){Serial.begin(115200);Serial1.begin(9600,SERIAL_8N1,17,18);sensor.begin(Serial1);}
void loop(){int ppm=sensor.getCO2();if(ppm>=0){Serial.print("CO2=");Serial.println(ppm);}else Serial.println("MH-Z19 read failed: check wiring, power and checksum");delay(250);}
