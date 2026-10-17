// Sender: GPIO17 TX -> receiver GPIO16 RX; shared GND.
void setup() {
  Serial.begin(115200);
  Serial1.begin(9600, SERIAL_8N1, 16, 17);
  Serial1.setTimeout(100);
  // The other board boots independently. Give its UART time to initialize.
  delay(100);
}
void loop() {
  Serial1.println("PING");
  String reply = Serial1.readStringUntil('\n');
  reply.trim();
  if (reply == "PONG") Serial.println("UART_OK");
  delay(1000);
}
