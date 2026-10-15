// Source OUT -> A0, source GND -> board GND. Raw ESP ADC accepts 0..1 V.
// Stop the simulation to adjust the source voltage in the inspector.
void setup() { pinMode(D5, OUTPUT); Serial.begin(115200); }
void loop() {
    const int value = analogRead(A0);
    digitalWrite(D5, value >= 512 ? HIGH : LOW);
    Serial.println(value);
    delay(200);
}
