# IR-, NFC- und Funkmodule in ThinkCad 1.0.0

Diese Bauteile ergänzen ESP32, Display, Taster und Stromversorgung für eigene
Flipper-artige Versuchsgeräte. Sie sind **keine Emulation der Flipper-Zero-CPU
oder seiner Firmware**. Der gewünschte Aufbau verwendet **ESP32 + PN532 + nRF24L01+**.
Ein nRF24 arbeitet bei 2,4 GHz und ersetzt kein Sub-GHz-CC1101-Modul. Zusätzlich
stehen eigenständige 433-MHz-ASK-Sender und -Empfänger bereit; CC1101 und RFID
bei 125 kHz sind nicht enthalten. Alle folgenden Modelle sind lokal simuliert.

Datei → Beispielschaltungen enthält fünf vollständig verdrahtete Versuche.
Simulation mit Strg+R starten; Ansicht → Serielle Werkzeuge zeigt die Ausgaben.
Die Quelltexte liegen in `examples/`. Im nRF24-Beispiel besitzt jedes ESP32-Board
einen eigenen Sketch; über die Auswahl oberhalb des Editors zwischen ihnen wechseln.

## Anschlüsse am ESP32

| Modul | Verbindung |
| --- | --- |
| IR-LED 940 nm | GPIO2 → 220 Ω → A; K → GND. Im Beispiel etwa 9 mA bei HIGH. Für stärkere echte IR-LEDs einen passenden Transistortreiber vorsehen. |
| IR-Empfänger 38 kHz | VCC → 3V3, GND → GND, OUT → GPIO4 |
| PN532 im I²C-Modus | VCC → 3V3, GND → GND, SDA → GPIO21, SCL → GPIO22, IRQ → GPIO27, RST → GPIO26 |
| nRF24L01+ | **VCC → 3V3**, GND → GND, SCK → GPIO18, MISO → GPIO19, MOSI → GPIO23, CSN → GPIO5, CE → GPIO16. IRQ darf für die Polling-API offen bleiben. Niemals 5 V an VCC. |
| 433-MHz-ASK-Sender | VCC → 3V3, GND → GND, DATA → GPIO25 |
| 433-MHz-ASK-Empfänger | VCC → 3V3, GND → GND, DATA → GPIO26 |

Die Beispiele sind getrennte Versuche. Für einen kombinierten Aufbau den ASK-
Empfänger umlegen, da GPIO26 im NFC-Beispiel bereits RST ist. Die Pinbelegung und
Betriebsspannung echter Breakouts können abweichen; insbesondere unterstützen
nicht alle realen ASK-Module einen zuverlässigen Betrieb mit 3,3 V.

## Infrarot und TV-Fernbedienung

- **IR: TV-Fernbedienung testen** lädt `ir_remote.ino`. Fernbedienung doppelt
  anklicken oder auswählen und **TV-Taste gedrückt** einschalten. Die Flanke
  beim Drücken sendet genau ein NEC-Kommando, standardmäßig Adresse 0, Kommando
  0x45 (69 dezimal). Zum erneuten Senden loslassen und wieder drücken.
- Kommando im Wertefeld ändern. Im Eigenschaften-Dock sind NEC-Adresse und
  Testgruppe einstellbar. **Übernehmen** speichert diese Einstellungen mit Undo.
- **IR: Senden und Empfangen** verwendet `ir_loopback.ino`: `IrSender.begin(2)`
  und `IrSender.sendNEC(address, command, repeats)` erreichen den Empfänger
  über eine tatsächlich versorgte, strombegrenzte IR-LED.
- `IrReceiver.begin(4)`, `decode()`, `decodedIRData` und `resume()` entsprechen
  der modernen Arduino-IRremote-API. Adresse, Kommando, NEC-Rohwert und Repeat-
  Flag sind verfügbar. Nach erfolgreichem Decode muss `resume()` folgen.

Sender/Remote und Empfänger benötigen dieselbe Testgruppe (0–15) und maximal
1.400 Szeneneinheiten Abstand. Das modelliert einen kontrollierbaren Testbereich,
keine reale Reichweite. Der Puffer hält acht Frames. Nur NEC wird unterstützt;
38-kHz-Träger, rohe Pulsfolgen, optische Leistung, Sichtkontakt und Störungen
werden nicht simuliert. OUT ist ein API-Endpunkt, keine mit digitalRead oder dem
Logikanalysator auswertbare demodulierte Pulsfolge. Die TV-Fernbedienung braucht
keine Kabel. Sie ist eine Testquelle, kein virtueller Fernseher.

## PN532 und kreditkartenförmige NFC-Testkarte

**NFC: PN532 und Testkarte** lädt `nfc_reader.ino`. `Wire.begin(21,22)` und
`Adafruit_PN532 nfc(27,26)` müssen zur Verdrahtung passen. `begin()`,
`getFirmwareVersion()`, `SAMConfig()`, `readPassiveTargetID()` und
`ntag2xx_ReadPage()` sind implementiert. RST muss HIGH sein; Versorgung und alle
vier Bus-/Steuerpins werden geprüft. Der Leser hat interne simulierte 4,7-kΩ-
Pullups nach VCC und die feste 7-Bit-I²C-Adresse 0x24.

Die kreditkartenförmige **NFC-Testkarte** trägt ausschließlich synthetische Daten:
standardmäßig UID `04:A1:B2:C3:D4:E5:F6` und NDEF-Text `ThinkCad NFC test card`.
Es handelt sich um einen ISO14443A-/Type-2-Test-Tag, **keine Bankkarten-/EMV-
Emulation**. Kreditkartennummern, Zahlung, Schlüssel und Authentifizierung sind
nicht modelliert. Die äußere Kartenform ändert das Protokoll nicht.

Karte auswählen: UID mit vier oder sieben Hex-Bytes und NDEF-Text mit maximal
96 UTF-8-Bytes eingeben, dann **Übernehmen**. Doppelklick oder **Karte am Leser**
steuert die Präsenz während des Laufs. Eine Karte muss innerhalb von 320
Szeneneinheiten zum Leser liegen und dieselbe Testgruppe besitzen. Keine oder
mehrere passende Karten ergeben keinen ausgewählten Tag. Das ist keine vollständige
Antikollisionssimulation. Die synthetischen Seiten 0–39 sind lesbar; ab Seite 4
steht ein NDEF-TLV mit Textrecord. Schreiben, Crypto1, APDUs und Raw-I²C fehlen.

## nRF24L01+: Sender und Empfänger

**Funk: nRF24L01 Sender und Empfänger** lädt zwei ESP32-Boards mit
`nrf24_pair.ino` (TX) und `nrf24_receiver.ino` (RX). Beide nutzen Kanal 76,
1 Mbit/s und die fünf Adressbytes `TCAD1`. Der Monitor zeigt `ACK:` und `RX:`.
Fehlen Empfänger, Stromversorgung, Empfangsmodus oder eine übereinstimmende
Konfiguration, erhält TX ohne deaktiviertes Auto-ACK `false`.

Unterstützte RF24-API: `RF24(ce,csn)`, `begin(&SPI)`, `setChannel`, `setDataRate`,
`setCRCLength`, `setAutoAck`, `setAddressWidth(5)`, `setPayloadSize`,
`enableDynamicPayloads`, `disableDynamicPayloads`, `openWritingPipe`,
`openReadingPipe`, `startListening`, `stopListening`, `write`, `available`,
`getDynamicPayloadSize` und `read`. Kanal 0–125 entspricht 2400 + Kanal MHz;
250 kbit/s, 1 und 2 Mbit/s sind möglich. Adressen sind fünf Bytes, Payloads 1–32
Bytes. Statische Payloads werden auf die konfigurierte Größe aufgefüllt; pro
Empfänger passen drei Pakete in den FIFO. Pakete werden erst nach modellierter
Luftzeit lesbar. CRC-Einstellung, Kanal, Datenrate und Adresse müssen passen.
Firmwarekonfiguration überschreibt den anfänglichen Kanal im Wertefeld.

Das Modell arbeitet mit logischen vollständigen Adressen je Pipe, nicht mit der
Register-/Adresspräfixorganisation des Chips. Auto-ACK meldet einen passenden,
versorgten Empfänger mit freiem FIFO und eingeschaltetem Auto-ACK. Es gibt keine
HF-Ausbreitung, Paketkollisionen, Rauschen, RSSI oder Bitfehler. `setPALevel` prüft
den Parameter, beeinflusst mangels Reichweitenmodell aber keine Dämpfung.
`setRetries` beeinflusst die Wartezeit bei fehlendem ACK; es simuliert keine
erneute Aussendung. ACK-Payloads, direkte Registerzugriffe, Interrupt- und
vollständige SPI-Transaktionen sind nicht implementiert. Hardwaretests bleiben nötig.

## ASK-Sender und -Empfänger

**Funk: 433-MHz-ASK-Paar** lädt `ask_pair.ino`. `RCSwitch` unterstützt
`enableTransmit(pin)`, `enableReceive(pin)`, `setProtocol(1)`, standardmäßige
350-µs-Pulse, `setRepeatTransmit(1…20)`, `send(value,bits)`, `available`,
`getReceivedValue`, `getReceivedBitlength`, `getReceivedProtocol` und
`resetAvailable`. Im ESP32-Profil entspricht die Interruptnummer dem GPIO.

Bis 32 Bit werden als Nachricht übertragen. Frequenzwerte beider Komponenten
müssen auf 0,01 MHz genau passen; dieselbe Testgruppe und maximal 3.000
Szeneneinheiten Abstand sind erforderlich. Acht logische Nachrichten passen in
den Puffer. Repeat-Anzahl beeinflusst die Sendedauer; eine Nachricht wird einmal
zugestellt. Keine reale Puls-/Träger- oder Störungsmodellierung; `digitalRead`
und Raw-Protokollanalysatoren ersetzen den Adapter nicht.

## Eigene Hardware kompilieren

Für echte Boards die Originalbibliotheken über den Bibliothekskatalog installieren:
**IRremote**, **Adafruit PN532** (mit **Adafruit BusIO**), **RF24** und **rc-switch**.
Alternativ mit bereits installiertem Arduino CLI:

```sh
arduino-cli lib update-index
arduino-cli lib install "IRremote" "Adafruit PN532" "RF24" "rc-switch"
arduino-cli core update-index --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli core install esp32:esp32 --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
```

Datei → Kompilieren / Auf Mikrocontroller laden verwendet den echten Board-Core
und diese Bibliotheken. Die Simulatoradapter unter `sdk/` werden dabei nicht
mitgeflasht. Board, Verdrahtung und Port prüfen; jedes Board einzeln hochladen.
Linux benötigt tty-Zugriff, Windows gegebenenfalls den USB-Treiber. Ein laufendes
ThinkCad-Modell ist kein Beleg für getestete Funkreichweite oder echte Hardware.
