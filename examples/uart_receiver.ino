// Receiver: a separate sketch on the second ESP32.
void setup() {
  Serial.begin(115200);
  Serial1.begin(9600, SERIAL_8N1, 16, 17);
}
void loop() {
  if (Serial1.available()) {
    String message = Serial1.readStringUntil('\n');
    message.trim();
    if (message == "PING") {
      Serial1.println("PONG");
      Serial.println("Received PING");
    }
  }
  delay(1);
}
