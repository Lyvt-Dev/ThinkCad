// ESP32 + PN532 NFC transmitter / reader-writer.
// VCC=3V3, GND=GND, SDA=21, SCL=22, IRQ=27, RST=26.
// Place the synthetic NFC test card near the PN532.
#include <Wire.h>
#include <Adafruit_PN532.h>
Adafruit_PN532 nfc(27, 26);
void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  if (!nfc.begin() || !nfc.SAMConfig()) Serial.println("PN532 not connected");
}
void loop() {
  uint8_t uid[7], length = 0;
  if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &length)) {
    uint8_t message[4] = {'T', 'E', 'S', 'T'};
    uint8_t result[4] = {};
    // Page 8 is a user page. UID, lock and capability pages are protected.
    if (nfc.ntag2xx_WritePage(8, message) && nfc.ntag2xx_ReadPage(8, result)
        && result[0] == 'T' && result[1] == 'E' && result[2] == 'S' && result[3] == 'T')
      Serial.println("NFC write verified");
  }
  delay(1000);
}
