// TMP36: VCC to 5V, OUT to A0, GND to GND.
// D9 through 220 ohms to LED anode, cathode to GND.
void setup() {
    Serial.begin(9600);
    pinMode(9, OUTPUT);
}
void loop() {
    float voltage = analogRead(A0) * (5.0f / 1023.0f);
    float temperatureC = (voltage - 0.5f) * 100.0f;
    Serial.println(temperatureC);
    digitalWrite(9, temperatureC > 20.0f ? HIGH : LOW);
    delay(100);
}
