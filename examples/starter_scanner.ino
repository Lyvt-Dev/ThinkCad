#include <Wire.h>
void setup() { Serial.begin(115200); Wire.begin(21, 22); }
void loop() {
  for (int address = 8; address < 120; ++address) {
    Wire.beginTransmission(address);
    if (Wire.endTransmission() == 0) {
      Serial.print("Found 0x"); Serial.println(address, HEX);
    }
  }
  delay(1000);
}
