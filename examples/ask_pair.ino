#include <RCSwitch.h>
// ASK protocol-1 loopback: transmitter DATA=GPIO25, receiver DATA=GPIO26.
RCSwitch transmitter, receiver;
void setup() {
  Serial.begin(115200); transmitter.enableTransmit(25); receiver.enableReceive(26);
  transmitter.setProtocol(1); transmitter.setRepeatTransmit(3);
}
void loop() {
  transmitter.send(0xA55A,16);
  if (receiver.available()) { Serial.print("ASK RX: 0x"); Serial.println(receiver.getReceivedValue(),HEX); receiver.resetAvailable(); }
  delay(600);
}
