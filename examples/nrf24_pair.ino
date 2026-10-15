#include <SPI.h>
#include <RF24.h>
// ESP32: SCK18 MISO19 MOSI23 CSN5 CE16; nRF24 VCC=3V3, never 5V.
RF24 radio(16, 5);
const byte address[6] = "TCAD1";
unsigned long counter=0;
void setup() {
  Serial.begin(115200); SPI.begin(18,19,23,5);
  if (!radio.begin()) { Serial.println("nRF24 missing"); while(true) delay(100); }
  radio.setChannel(76); radio.setDataRate(RF24_1MBPS); radio.setPALevel(RF24_PA_LOW);
  radio.enableDynamicPayloads(); radio.openWritingPipe(address); radio.stopListening();
}
void loop() {
  char message[32]; snprintf(message,sizeof(message),"ThinkCad packet %lu",counter++);
  bool acknowledged=radio.write(message,strlen(message)+1);
  Serial.print(acknowledged ? "ACK: " : "No ACK: "); Serial.println(message);
  delay(700);
}
