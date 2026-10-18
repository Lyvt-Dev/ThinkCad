void setup() {
  for (int pin = 4; pin <= 9; ++pin) pinMode(pin, OUTPUT);
  digitalWrite(4, HIGH); digitalWrite(7, LOW);
  digitalWrite(6, LOW); digitalWrite(8, LOW); digitalWrite(9, LOW);
  analogWrite(5, 160);
}
void loop() { delay(100); }
