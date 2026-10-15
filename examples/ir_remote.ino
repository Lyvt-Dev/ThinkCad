#include <IRremote.hpp>
// ESP32 GPIO4 <- OUT of the 38 kHz IR receiver.
// Double-click the TV remote, or use "TV button pressed" in the inspector.
void setup() { Serial.begin(115200); IrReceiver.begin(4, DISABLE_LED_FEEDBACK); }
void loop() {
  if (IrReceiver.decode()) {
    Serial.print("NEC address=0x"); Serial.print(IrReceiver.decodedIRData.address, HEX);
    Serial.print(" command=0x"); Serial.println(IrReceiver.decodedIRData.command, HEX);
    IrReceiver.resume();
  }
  delay(10);
}
