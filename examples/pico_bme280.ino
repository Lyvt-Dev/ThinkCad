#include <Adafruit_BME280.h>
Adafruit_BME280 sensor;
void setup() {
  Serial.begin(115200);
  // Arduino-Pico core: GPIO0 = SDA, GPIO1 = SCL.
  Wire.setSDA(0);
  Wire.setSCL(1);
  Wire.begin();
  if (!sensor.begin(0x76, &Wire)) {
    Serial.println("BME280 not found; check power and wiring");
    return;
  }
  Serial.println("BME280_READY");
}
void loop() {
  Serial.print("T="); Serial.print(sensor.readTemperature());
  Serial.print(" C  RH="); Serial.print(sensor.readHumidity());
  Serial.print(" %  P="); Serial.print(sensor.readPressure() / 100.0f);
  Serial.println(" hPa");
  delay(1000);
}
