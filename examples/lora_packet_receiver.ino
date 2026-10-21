// Second ESP32 + SX1276, same wiring as lora_packet_sender.ino.
#include <SPI.h>
const int CS = 5;
uint8_t accessRegister(uint8_t reg, uint8_t value) {
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
  digitalWrite(CS, LOW);
  SPI.transfer(reg);
  uint8_t result = SPI.transfer(value);
  digitalWrite(CS, HIGH);
  SPI.endTransaction();
  return result;
}
void writeRegister(uint8_t reg, uint8_t value) { accessRegister(reg | 0x80, value); }
uint8_t readRegister(uint8_t reg) { return accessRegister(reg, 0); }
void setup() {
  Serial.begin(115200);
  pinMode(CS, OUTPUT);
  digitalWrite(CS, HIGH);
  SPI.begin(18, 19, 23);
  writeRegister(1, 0x80);
  writeRegister(6, 0xd9);
  writeRegister(7, 0);
  writeRegister(8, 0);
  writeRegister(0x0f, 0); // RX FIFO base
  writeRegister(0x1d, 0x72);
  writeRegister(0x1e, 0x74);
  writeRegister(0x39, 0x12);
  writeRegister(1, 0x85); // continuous receive
}
void loop() {
  if (readRegister(0x12) & 0x40) {
    writeRegister(0x0d, readRegister(0x10));
    uint8_t count = readRegister(0x13);
    for (unsigned i = 0; i < count; ++i) Serial.write(readRegister(0));
    Serial.println();
    writeRegister(0x12, 0xff);
  }
  delay(10);
}
