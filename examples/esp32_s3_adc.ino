void setup() { pinMode(5, OUTPUT); }
void loop() { digitalWrite(5, analogRead(A0) >= 2000 ? HIGH : LOW); delay(40); }
