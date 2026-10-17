constexpr int triggerPin = 2, echoPin = 3;
void setup() {
  Serial.begin(115200);
  pinMode(triggerPin, OUTPUT);
  pinMode(echoPin, INPUT);
  digitalWrite(triggerPin, LOW);
}
void loop() {
  digitalWrite(triggerPin, LOW); delayMicroseconds(2);
  digitalWrite(triggerPin, HIGH); delayMicroseconds(10);
  digitalWrite(triggerPin, LOW);
  const unsigned long width = pulseIn(echoPin, HIGH, 30000);
  if (width) {
    Serial.print("Distance (cm): "); Serial.println(width * 0.0343f / 2.0f);
  } else Serial.println("No echo");
  delay(100); // HC-SR04 needs at least 60 ms between measurements.
}
