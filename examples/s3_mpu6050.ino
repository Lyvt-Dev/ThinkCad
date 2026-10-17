#include <Adafruit_MPU6050.h>
Adafruit_MPU6050 sensor;
void setup() {
  Serial.begin(115200);
  Wire.begin(8, 9); // ESP32-S3 GPIO8 = SDA, GPIO9 = SCL.
  if (!sensor.begin(0x68, &Wire)) {
    Serial.println("MPU6050 not found; check power and wiring");
    return;
  }
  sensor.setAccelerometerRange(MPU6050_RANGE_8_G);
  sensor.setGyroRange(MPU6050_RANGE_500_DEG);
  Serial.println("MPU6050_READY");
}
void loop() {
  sensors_event_t acceleration, rotation, temperature;
  sensor.getEvent(&acceleration, &rotation, &temperature);
  Serial.print("Acceleration X (m/s^2): "); Serial.println(acceleration.acceleration.x);
  Serial.print("Angular velocity X (rad/s): "); Serial.println(rotation.gyro.x);
  Serial.print("Temperature (C): "); Serial.println(temperature.temperature);
  delay(250);
}
