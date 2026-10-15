// Four LEDs with individual 220-ohm resistors, cathodes to GND.
const int leds[] = {D1, D2, D5, D6};
void setup() { for (int pin : leds) pinMode(pin, OUTPUT); }
void loop() {
    for (int pin : leds) {
        digitalWrite(pin, HIGH); delay(200);
        digitalWrite(pin, LOW);
    }
}
