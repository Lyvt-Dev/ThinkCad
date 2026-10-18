void setup() { Serial.begin(115200); pinMode(5, OUTPUT); }
void loop() {
  const int reading = analogRead(A0);
  analogWrite(5, reading / 4);
  Serial.println(reading);
  delay(100);
}
