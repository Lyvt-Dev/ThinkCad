#include <IRremote.hpp>
// GPIO2 -> 220 ohm -> IR LED A, IR LED K -> GND. Receiver OUT -> GPIO4.
void setup() {
  Serial.begin(115200);
  IrReceiver.begin(4, DISABLE_LED_FEEDBACK);
  IrSender.begin(2);
}
void loop() {
  IrSender.sendNEC(0x00, 0x45, 0);
  if (IrReceiver.decode()) {
    Serial.print("IR loopback command=0x"); Serial.println(IrReceiver.decodedIRData.command, HEX);
    IrReceiver.resume();
  }
  delay(800);
}
