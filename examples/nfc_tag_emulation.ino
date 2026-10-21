// ESP32 + PN532, paired with nfc_tag_peer.ino in the same project.
// VCC=3V3, GND=GND, SDA=21, SCL=22, IRQ=27, RST=26.
#include <Wire.h>
#include <Adafruit_PN532.h>
Adafruit_PN532 nfc(27, 26);
bool reported = false;
void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  const uint8_t uid[7] = {0x04, 0xa1, 0xb2, 0xc3, 0xd4, 0xe5, 0xf6};
  // thinkcad* methods are simulator extensions, not upstream Adafruit APIs.
  if (nfc.begin() && nfc.thinkcadBeginTagEmulation(uid, 7, "ThinkCad board tag", true))
    Serial.println("Tag emulation ready");
}
void loop() {
  uint8_t page[4];
  if (!reported && nfc.thinkcadReadEmulatedPage(8, page)
      && page[0] == 'P' && page[1] == 'I' && page[2] == 'N' && page[3] == 'G') {
    Serial.println("Emulated tag received PING");
    reported = true;
  }
  delay(20);
}
