#include <Wire.h>
#include <Adafruit_PN532.h>
// PN532 I2C: SDA21, SCL22, IRQ27, RST26, VCC3V3, GND.
// Synthetic test card only: ISO14443A UID + read-only Type-2 NDEF text.
Adafruit_PN532 nfc(27, 26);
void setup() {
  Serial.begin(115200); Wire.begin(21, 22); nfc.begin();
  if (!nfc.getFirmwareVersion() || !nfc.SAMConfig()) { Serial.println("PN532 missing"); while (true) delay(100); }
  Serial.println("PN532 ready: present the test card");
}
void loop() {
  uint8_t uid[7], length;
  if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &length, 100)) {
    Serial.print("Test UID:"); for (uint8_t i=0;i<length;++i) { Serial.print(' '); Serial.print(uid[i], HEX); } Serial.println();
    uint8_t page[4];
    if (nfc.ntag2xx_ReadPage(4, page)) { Serial.print("NDEF TLV: 0x"); Serial.println(page[0], HEX); }
  } else Serial.println("No test card");
  delay(700);
}
