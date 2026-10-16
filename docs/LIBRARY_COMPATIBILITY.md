# Kompatibilität der eingebauten Bibliotheken

Die Simulatorbibliotheken sind native C++-Adapter. Sie ersetzen weder die
Original-Firmware eines Boards noch sämtliche Register aller Peripheriebausteine.
Die folgenden Prüfungen laufen mit den Boarddefinitionen von ESP8266, ESP32,
ESP32-S3, Uno R3 und klassischem Nano.

| Bibliothek | Geprüft / behoben | Verbleibende Grenze |
| --- | --- | --- |
| Wire | Master-Schreib-/Lesezugriffe, Byte-/String-/Puffer-Overloads, ACK/NACK, Pufferüberlauf, Register-Lesezugriffe mit wiederholtem Start, boardabhängige Pins und Puffer, simulierter Bustakt; Stream-API | Ein Controller, kein Slave-Sketch und kein Clock-Stretching; ein Gerät braucht ein implementiertes Modell |
| Stream / Serial | `readBytes`, `readBytesUntil`, `readString`, `readStringUntil`, `parseInt`, `parseFloat`, `find`, `findUntil`, Lookahead und Timeout; Serial und Wire sind Streams | Serial-Monitor-Verbindung, keine vollständige UART-Registeremulation; Arduino-String ist weiterhin ein `std::string`-Alias |
| LiquidCrystal | Text, Cursor, Anzeige-/Entry-/Shift-Kommandos, DDRAM-/CGRAM-Adressen, zwei 40-Zeichen-Bänke, 16×2/20×4, selbst definierte Zeichen; erneutes `begin` setzt den Zustand zurück | Parallele Komponente stellt D4–D7 bereit, kein 8-Bit-Bus |
| LiquidCrystal_I2C | Obige Text-API, `init`, `begin`, öffentliches `setBacklight`; zusätzlich Rohzugriff über Wire auf den PCF8574 | Backpack-Belegung RS/RW/E/Backlight=P0–P3 und D4–D7=P4–P7 |
| Adafruit SSD1306 / GFX | GFX-Zeichenkern und Grafiktests, `begin()` mit Standardparametern und automatischer Adresse (128×32: 0x3c, 128×64: 0x3d), explizite Adresse weiterhin möglich | 128×32/64 und I²C; Hardware-Scrolling über den Adapter vereinfacht, nicht über rohe Scroll-Kommandos |
| Servo | Attach/Detach, Winkel/Pulsbreite und Erhalt einer vor `attach` gesetzten Position; bestehender Sketch-/Physiktest | Keine mechanische Last oder Timer-Registeremulation |
| PN532 | Bestehender Sketchtest: UID, Type-2-NDEF, Anwesenheit und Versorgung; Null-Bus wird verständlich abgewiesen | ISO14443A-Testkarte, kein vollständiges PN532-Rohprotokoll |
| RF24 | Bestehender Sketchtest: Senden/Empfangen, Adressen, Datenraten, Kanal, CRC, ACK und FIFO; Null-Bus/-Adressen werden abgewiesen | Kein vollständiger nRF24-Registerzugriff; Paketmodell und begrenzte RF24-API |
| IRremote | Bestehende Sketchtests: NEC Senden/Empfangen, Repeat und Resume | NEC-Adapter, keine allgemeine IR-Pulsform-/Protokollbibliothek |
| rc-switch | Bestehender Sketchtest: ASK-Senden/Empfangen und Frequenzabgleich | Protokoll 1, 350-µs-Basispuls, Nachrichtenmodell |
| SPI | Boardabhängige Pininitialisierung, verwendet vom RF24-Adapter | **Rohes `SPI.transfer` ist weiterhin nicht implementiert.** Transaktionsparameter werden bisher nicht als Buswellenform simuliert. |

## LCD-Text über I²C

`Wire.write("hello world")` schreibt ASCII-Bytes auf den Bus. Ein LCD-Backpack
interpretiert diese Bytes als Zustände seiner acht Ausgangspins, nicht als Text.
Für Text `LiquidCrystal_I2C` verwenden oder selbst Initialisierung, RS/E und die
Daten-Nibbles des HD44780 übertragen. Ein vollständiger Sketchtest prüft beide
Fälle und liest den Port zurück. Ein falsches oder unversorgtes Gerät erhält
keine erfolgreiche Bestätigung; doppelte Adressen ergeben einen Busfehler.

`createChar()` hinterlässt den LCD-Adresszeiger im CGRAM, wie bei der
Arduino-Bibliothek. Vor anschließendem Text `setCursor()` aufrufen. Das
mitgelieferte Beispiel wurde entsprechend korrigiert.

## Validierung

`sdk_wire` und `sdk_wire_avr` testen die Wire-Transaktionen und Fehlerfälle.
`sdk_compatibility_*` prüft die gemeinsamen Header und Zustandsübergänge auf allen
fünf Boardprofilen. `wireless_peripherals`, `professional_workflows`, `display_api`
und die Desktoptests prüfen zusätzlich kompilierte Sketches und Geräteverhalten.
Diese Tests sind keine Zusage vollständiger Kompatibilität mit beliebigen
Arduino-Bibliotheken. Die oben genannten offenen Modellgrenzen bleiben sichtbar.
