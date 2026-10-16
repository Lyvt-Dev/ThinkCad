# ThinkCad 1.2.3

- PWM-fähige Pins sind auf Uno R3, Nano, ESP8266, ESP32 und ESP32-S3 mit `~`
  beschriftet, sowohl im Block- als auch im realistischen Design. Elektrische
  Anschlussnamen bleiben kompatibel mit vorhandenen Projekten.
- Wire-Master-API mit gepufferten Byte-/String-/Puffer-Schreibzugriffen,
  `endTransmission`, `requestFrom`, ACK/NACK, Pufferüberlauf und wiederholtem
  Start. Standardpins und Puffergrößen folgen dem gewählten Board.
- Rohes I²C für PCF8574-LCD-Backpacks und SSD1306 mit Prüfung von Verdrahtung,
  Versorgung und Adresse. LCD-Text benötigt weiterhin LCD-Kommandos oder
  LiquidCrystal_I2C; reine ASCII-Bytes sind keine LCD-Textbefehle.
- Serial und Wire unterstützen Stream-Funktionen einschließlich Lesen,
  Zahleneingabe, Suche und Timeouts. String-/Puffer-Overloads wurden ergänzt.
- LiquidCrystal unterstützt zusätzliche Standardkommandos, DDRAM-/CGRAM-Zugriffe
  und korrektes Bank-Wrapping. Das I²C-Beispiel setzt nach `createChar` den Cursor.
- OLED-Initialisierung mit Standardparametern und automatischer Adresse wie in
  Adafruit SSD1306: 128×32 verwendet 0x3c, 128×64 verwendet 0x3d. Eine explizite
  Adresse wie `display.begin(SSD1306_SWITCHCAPVCC, 0x3c)` bleibt möglich.
- Servo erhält eine vor `attach` gesetzte Position. Ungültige PN532-/RF24-Buszeiger
  werden mit einer Fehlermeldung statt einem Absturz abgewiesen.
- Neue Wire-Fehlerfalltests, Kompatibilitätstests für alle fünf Boardprofile und
  vollständige Sketchtests für I²C-LCD und serielles Parsing. Lokal bestehen alle
  24 Testsuiten.

Diese Version ist keine vollständige Arduino-Core- oder Geräteemulation. Rohes
SPI, I²C-Slave-Sketches, Clock-Stretching und einige Geräteprotokolle sind weiterhin
nicht implementiert. Die genauen Grenzen stehen in
[LIBRARY_COMPATIBILITY.md](LIBRARY_COMPATIBILITY.md).

Windows-Installer, Linux-Pakete, portable AppImage und Gentoo-Overlay werden aus
identischem Quellstand erstellt. Für Gentoo ist die AppImage eine Alternative
zum nativen Portage-Build; Installations- und Updatebefehle stehen in der README.
