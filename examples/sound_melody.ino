// Piezo + on D3, - on GND.
// Speaker + through 470 ohms to D3, - on GND.
const unsigned int notes[] = {262, 294, 330, 349, 392, 440, 494, 523};
void setup() {}
void loop() {
    for (unsigned int note : notes) {
        tone(3, note, 180);
        delay(220);
    }
    noTone(3);
    delay(500);
}
