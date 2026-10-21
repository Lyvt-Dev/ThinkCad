// Second ESP32 + PN532, same pin wiring as nfc_tag_emulation.ino.
#include <Wire.h>
#include <Adafruit_PN532.h>
Adafruit_PN532 nfc(27, 26);
void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  nfc.begin();
  nfc.SAMConfig();
}
void loop() {
  uint8_t uid[7], length = 0;
  const uint8_t expected[7] = {0x04, 0xa1, 0xb2, 0xc3, 0xd4, 0xe5, 0xf6};
  if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &length)) {
    bool matched = length == 7;
    for (unsigned i = 0; i < length; ++i) if (uid[i] != expected[i]) matched = false;
    uint8_t message[4] = {'P', 'I', 'N', 'G'}, result[4];
    if (matched && nfc.ntag2xx_WritePage(8, message) && nfc.ntag2xx_ReadPage(8, result)
        && result[0] == 'P' && result[1] == 'I' && result[2] == 'N' && result[3] == 'G')
      Serial.println("Board tag write verified");
  }
  delay(200);
}
