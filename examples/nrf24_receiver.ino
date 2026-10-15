#include <SPI.h>
#include <RF24.h>
RF24 radio(16,5);
const byte address[6]="TCAD1";
void setup() {
  Serial.begin(115200); SPI.begin(18,19,23,5);
  if (!radio.begin()) { Serial.println("nRF24 missing"); while(true) delay(100); }
  radio.setChannel(76); radio.setDataRate(RF24_1MBPS); radio.setPALevel(RF24_PA_LOW);
  radio.enableDynamicPayloads(); radio.openReadingPipe(1,address); radio.startListening();
}
void loop() {
  if (radio.available()) { char message[33]={0}; radio.read(message,32); Serial.print("RX: "); Serial.println(message); }
  delay(10);
}
