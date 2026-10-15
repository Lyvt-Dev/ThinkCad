// Button between D6 and GND. LED with 220 ohms between D5 and GND.
void setup() { pinMode(D6, INPUT_PULLUP); pinMode(D5, OUTPUT); }
void loop() {
    digitalWrite(D5, digitalRead(D6) == LOW ? HIGH : LOW);
    delay(20);
}
