#include <DHT.h>
DHT sensor(2, DHT22);
void setup() { Serial.begin(115200); sensor.begin(); }
void loop() {
  delay(2100);
  Serial.print("Temperature: "); Serial.println(sensor.readTemperature());
  Serial.print("Humidity: "); Serial.println(sensor.readHumidity());
}
