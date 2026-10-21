// ESP32 + SX1276: VCC=3V3, GND=GND, SCK=18, MISO=19, MOSI=23, CS=5.
// RESET has the simulated module's pull-up. Pair with lora_packet_receiver.ino.
#include <SPI.h>
const int CS = 5;
void writeRegister(uint8_t reg, uint8_t value) {
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
  digitalWrite(CS, LOW);
  SPI.transfer(reg | 0x80);
  SPI.transfer(value);
  digitalWrite(CS, HIGH);
  SPI.endTransaction();
}
void setup() {
  pinMode(CS, OUTPUT);
  digitalWrite(CS, HIGH);
  SPI.begin(18, 19, 23);
  writeRegister(0x01, 0x80); // LoRa sleep
  writeRegister(0x06, 0xd9); // 868 MHz
  writeRegister(0x07, 0x00);
  writeRegister(0x08, 0x00);
  writeRegister(0x0e, 0x00); // TX FIFO base
  writeRegister(0x1d, 0x72); // 125 kHz, coding rate 4/5, explicit header
  writeRegister(0x1e, 0x74); // SF7, CRC on
  writeRegister(0x39, 0x12); // private sync word
  writeRegister(0x01, 0x81); // standby
}
void loop() {
  const char message[] = "Hello ThinkCad";
  writeRegister(0x0d, 0);
  for (unsigned i = 0; i < sizeof(message) - 1; ++i) writeRegister(0, message[i]);
  writeRegister(0x22, sizeof(message) - 1);
  writeRegister(0x12, 0xff);
  writeRegister(0x01, 0x83); // transmit, then return to standby
  delay(1000);
}
