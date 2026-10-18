int brightness = 0, direction = 5;
void setup() { pinMode(5, OUTPUT); }
void loop() {
  analogWrite(5, brightness);
  brightness += direction;
  if (brightness >= 255 || brightness <= 0) direction = -direction;
  delay(20);
}
