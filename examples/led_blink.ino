// D5 -> 220 ohms -> LED A; LED K -> GND.
void setup() { pinMode(D5, OUTPUT); }
void loop() {
    digitalWrite(D5, HIGH); delay(500);
    digitalWrite(D5, LOW); delay(500);
}
