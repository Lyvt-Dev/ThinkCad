// Three LEDs, each with its own 220-ohm resistor: red D5, amber D6, green D7.
const int red = D5, amber = D6, green = D7;
void setup() {
    pinMode(red, OUTPUT); pinMode(amber, OUTPUT); pinMode(green, OUTPUT);
}
void loop() {
    digitalWrite(red, HIGH); delay(2000);
    digitalWrite(amber, HIGH); delay(600);
    digitalWrite(red, LOW); digitalWrite(amber, LOW);
    digitalWrite(green, HIGH); delay(2000);
    digitalWrite(green, LOW); digitalWrite(amber, HIGH); delay(600);
    digitalWrite(amber, LOW);
}
