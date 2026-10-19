#include <Wire.h>
#include <Adafruit_BME680.h>
Adafruit_BME680 sensor;
void setup(){Serial.begin(115200);Wire.begin(8,9);if(!sensor.begin(0x76)){Serial.println("BME680 not found: check power, address and SDA/SCL");while(true)delay(1000);}sensor.setGasHeater(320,150);}
void loop(){if(sensor.performReading()){Serial.print("Temperature=");Serial.println(sensor.temperature);Serial.print("Humidity=");Serial.println(sensor.humidity);Serial.print("Pressure Pa=");Serial.println(sensor.pressure);Serial.print("Simulated gas resistance ohm=");Serial.println(sensor.gas_resistance);}else Serial.println("BME680 read failed");delay(250);}
