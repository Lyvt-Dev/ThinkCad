# Funk-, Schnittstellen- und Akkumodelle

Die Modelle arbeiten ausschließlich innerhalb des ThinkCad-Projekts. Sie senden
keine Funkpakete ins Netzwerk oder an echte Geräte. Die Hinweise in den
Komponenteneigenschaften beschreiben den jeweiligen Simulationsumfang.

## Bereits vorhanden

- nRF24L01: paketbasierter 2,4-GHz-Transceiver mit RF24-API.
- PN532: I²C-NFC-Leser mit synthetischen Testkarten; RC522: SPI/ISO14443A.
  Beide sind **13,56-MHz-Geräte**, keine 125-kHz-Leser. Testkarten sind keine
  Bankkarten- oder vollständigen Zugangskartenmodelle.
- IR-LED (940 nm), IR-Empfänger (38 kHz), NEC-Testfernbedienung.
  Diese generischen Modelle decken die grundlegenden Funktionen von IR-Emittern
  und demodulierenden Empfängern wie LTE-302 bzw. TSOP38238 ab; exakte optische
  Kennlinien und alle Fernbedienungsprotokolle werden nicht nachgebildet.
- ESP8266 und ESP32 einschließlich mehrerer Boardvarianten; Sketch-Ausführung.
  Daraus folgt keine vollständige Wi-Fi-/BLE-Protokoll- oder Funkchipemulation.
- USB-C-Stromanschluss: vorhandener passiver Steckverbinder. Für einen Ladeeingang
  gibt es zusätzlich den unten beschriebenen Anschluss mit CC-Widerständen.

## Neue Modelle

| Suchbegriff | Simulation |
| --- | --- |
| NFC transmitter / reader-writer · PN532 | Aktiver 13,56-MHz-Leser/Schreiber mit Adafruit_PN532-Adapter. UID und NTAG-Seiten lesen, Benutzerseiten 4–39 schreiben; synthetischer Tag-Emulationsmodus für einen zweiten PN532. |
| CC1101 | SPI-Konfigurationsregister, Status, Reset/RX/TX/Flush-Strobes, 64-Byte-FIFOs, feste und variable Paketlänge, Adressfilter und angehängter Empfangsstatus. Sender/Empfänger benötigen passende Frequenzregister, Kanal, Sync-Wort und Modemkonfiguration. |
| SX1276 / RFM95 | LoRa-Register, FIFO-Zeiger, Paketlänge, TxDone/RxDone, kontinuierlicher und einzelner Empfang. Frequenz, Sync-Wort und Modemkonfiguration müssen übereinstimmen. |
| MCP2515 | SPI-Reset, Registerzugriff, Bit Modify, CAN-Moduswahl, TX-Puffer und RTS; Loopback in RXB0/RXB1 mit Interruptflags. Externe CAN-Übertragung und ACK/Arbitration sind noch nicht implementiert. |
| TJA1050 | Elektrische dominant/recessive CANH/CANL-Ausgänge, RXD und Silent-Modus. Versorgung 5 V. MCP2515 ist der Controller, TJA1050 der physische Transceiver. |
| SIM800L | Offline-Testmodem, 9600 Baud/8N1, Versorgung 3,4–4,4 V, 2,8-V-Logik. |
| SIM7600 | **UART-Trägerplatine** mit 5-V-Eingang und 3,3-V-Pegelübersetzung, 115200 Baud/8N1. Kein Modell des nackten 1,8-V-Logikchips. |
| NEO-6M | Synthetische Position, Höhe und Fix-Status; 9600 Baud/8N1. NMEA-Abfrage liefert GPRMC/GPGGA mit Prüfsummen. |
| Optical fingerprint sensor | EF01-UART-Testprotokoll, 57600 Baud/8N1. GetImage, Image2Tz, Search, VerifyPassword und TemplateCount. Finger, Treffer und Template-ID sind einstellbar. |
| Li-ion 18650 / LiPo 1S / NiMH AA | Spannungsquelle mit Innenwiderstand und einstellbarem Ladezustand; Lithium 3,0–4,2 V, NiMH 1,0–1,4 V. |
| USB-C charging | Passiver 5-V-Sink-Breakout mit je 5,1 kΩ von CC1/CC2 nach GND. VBUS benötigt eine externe Versorgung. |
| TP4056 | DC-Näherung eines 1S-Lithium-Laders: einstellbare Stromgrenze, 4,2-V-Spannungsgrenze, gemeinsame Masse, Open-Drain-Ladestatus. |

Funkmodelle sind Paketmodelle ohne physikalische Ausbreitung, Störungen, exakte
Sendezeit oder vollständiges Register-/Treiberverhalten. SX1276-FSK und CC1101-
GDO-Konfiguration sind nicht implementiert. MCP2515-Interruptflags sind über SPI
lesbar; die neuen digitalen IRQ-Ausgänge werden noch nicht elektrisch getrieben.
Die Akkumodelle ändern ihren Ladezustand **nicht automatisch** durch Entladen oder
Laden. Kapazität, Temperatur, Alterung und Ladeablauf sind noch nicht modelliert.
TP4056 enthält keinen simulierten Tiefentlade-/Kurzschlussschutz.

### UART anschließen

TX des Moduls an RX des Mikrocontrollers, RX des Moduls an dessen TX, GND verbinden
und eine zum Modell passende Versorgung verwenden. Die konfigurierten Baudraten
und 8N1 müssen stimmen. Versorgung und Eingangsspannungspegel werden geprüft.
SIM800L benötigt z. B. vom 3,3-V-GPIO einen Spannungsteiler: 1 kΩ vom GPIO nach RX,
5,6 kΩ von RX nach GND. Sein 2,8-V-TX wird vom simulierten ESP32 als HIGH erkannt.

Die Modem-Testbefehle sind `AT`, `ATI`, `ATE0`, `ATE1`, `AT+CPIN?`, `AT+CSQ`,
`AT+CREG?`, `AT+CMGF=1` und `AT+CMGL="ALL"`. Die optionale SMS ist ein fester
synthetischer Testtext; unbekannte Befehle liefern `ERROR`.

Für den GNSS-Test die ASCII-Abfrage `$PUBX,00*33\r\n` senden. Der Empfänger liefert
zwei NMEA-Zeilen. Koordinaten und Fix-Status ändern sich über Eigenschaften. Es gibt
noch keinen periodischen NMEA-Sendebetrieb und keine UBX-Konfiguration. UTC zählt
ab Simulationsstart, das Testdatum ist fest auf 01.01.2000 gesetzt.

### USB-C und Lithiumzelle anschließen

Externe 5 V an USB-C `VBUS`, Versorgung-Minus an `GND`. `VBUS` mit TP4056 `VCC`
verbinden, Massen verbinden. Die **einzelne** Li-ion/LiPo-Zelle kommt an `BAT+`
und `BAT-`. `CHRG`/`STDBY` benötigen externe Pull-ups. Dieses Modell ist kein
NiMH-Lader, kein Mehrzellenlader und kein Ersatz für eine reale Schutzschaltung.

## Neue Planungskomponenten ohne Firmware-/RF-Backend

- **nRF52840 / MDBT50Q**: Modulabstraktion mit ausgewählten GPIO-, SWD- und USB-
  Anschlüssen. Kein vollständiges Pad-Layout, ausführbares Board oder BLE-Stack.
- **RTL2832U SDR-Dongle**: Planung einer USB-Dongle-Baugruppe mit Tuner. Der
  RTL2832U allein ist kein RF-Tuner. Keine USB-Enumeration oder IQ-Aufzeichnung.
- **AD9361 SDR-Modul**: Planungsabstraktion einer versorgten Modulbaugruppe,
  kein nacktes BGA-Pinout. Keine IQ-Datenpfade oder SPI-Registeremulation.

Diese Einträge tragen ausdrücklich „planning only“ im Namen und weisen in ihren
Eigenschaften auf die fehlende Simulation hin. Sie dürfen nicht mit vollständig
emulierten Bauteilen verwechselt werden.

## LoRa-Beispiel mit zwei ESP32

Das Beispiel **LoRa: ESP32 + SX1276** ist direkt im Beispieldialog verfügbar.
Für einen eigenen Aufbau je einen SX1276 an jeden ESP32 anschließen: `VCC → 3V3`, `GND → GND`,
`SCK → GPIO18`, `MISO → GPIO19`, `MOSI → GPIO23`, `CS → GPIO5`.
Der Reset-Pin besitzt im Modulmodell einen Pull-up. Beide Module im selben
Projekt platzieren. Das erste Board erhält
[den Sendesketch](../examples/lora_packet_sender.ino), das zweite
[den Empfangssketch](../examples/lora_packet_receiver.ino). Beide konfigurieren
868 MHz, SF7, 125 kHz und Sync-Wort 0x12. Im seriellen Monitor des zweiten Boards
erscheint einmal pro Sekunde `Hello ThinkCad`.


## NFC-Transmitter / Reader-Writer finden

In der Bauteilsuche `NFC transmitter` oder `PN532` eingeben. Der neue Eintrag
**NFC transmitter / reader-writer · PN532** kann die synthetische NFC-Testkarte
lesen und beschreiben. Das vorhandene PN532-Lesermodell unterstützt ebenfalls
`ntag2xx_WritePage(page, data)`; es ist dieselbe aktive NFC-Sendefunktion, kein
separater Funkstandard. Im Beispieldialog gibt es **NFC transmitter: PN532
reader-writer** mit vollständiger Verkabelung und überprüftem Schreib-/Lese-Sketch.

Schreibbar sind vier Bytes je Benutzerseite von 4 bis 39. UID-, Lock- und
Capability-Seiten bleiben geschützt. Änderungen gelten nur für die laufende
Simulation und werden beim Neustart zurückgesetzt. Der Tag muss aufliegen,
in Reichweite sein und vorher über `readPassiveTargetID` ausgewählt werden.
Zusätzlich unterstützt dasselbe Bauteil den unten beschriebenen synthetischen
Tag-Emulationsmodus. Peer-to-Peer, Zahlungskartenprotokolle und 125-kHz-RFID sind
nicht implementiert.


## NFC-Tag von einem zweiten Mikrocontroller emulieren

Das Beispiel **NFC: two-board tag emulation** enthält zwei ESP32 mit je einem
PN532. Es wird keine separate Testkarte benötigt: Der erste ESP32 stellt eine
synthetische Karte bereit, der zweite liest ihre UID, schreibt `PING` in Seite 8
und liest die Daten zurück. Der erste ESP32 bestätigt die tatsächlich empfangene
Änderung im seriellen Monitor mit `Emulated tag received PING`.

Die Verdrahtung ist bei beiden Boards `VCC → 3V3`, `GND → GND`, `SDA → GPIO21`,
`SCL → GPIO22`, `IRQ → GPIO27`, `RST → GPIO26`. Die beiden PN532 müssen innerhalb
der simulierten NFC-Reichweite liegen. Eine zusätzliche aufliegende Testkarte
kann eine Kollision verursachen; bei mehreren passenden Zielen wird keines
gewählt. Ein einzelner PN532 ist entweder Leser/Schreiber oder emulierter Tag.

Die folgenden Methoden sind ausdrücklich **ThinkCad-Simulationserweiterungen**
der mitgelieferten `Adafruit_PN532`-Klasse, keine APIs der upstream Adafruit-
Bibliothek und keine direkt auf echte PN532-Hardware übertragbare Firmware:

| Methode | Funktion |
| --- | --- |
| `thinkcadBeginTagEmulation(uid, length, text, writable)` | Tag mit 4- oder 7-Byte-UID, bis zu 96 UTF-8-Textbytes und optionalem Schreibschutz bereitstellen; `writable` ist standardmäßig `true`. |
| `thinkcadStopTagEmulation()` | Tag zurückziehen und zum Leserbetrieb zurückkehren. |
| `thinkcadTagEmulationActive()` | Prüfen, ob der eigene PN532 noch einen Tag bereitstellt. |
| `thinkcadReadEmulatedPage(page, data)` | Vier Bytes des eigenen emulierten Speichers lesen, einschließlich Änderungen des anderen Boards. |
| `thinkcadWriteEmulatedPage(page, data)` | Benutzerseite 4–39 vom eigenen Sketch aktualisieren; der entfernte Leser sieht die Änderung. |

Beispiel für den bereitstellenden Sketch nach `nfc.begin()`:

```cpp
const uint8_t uid[7] = {0x04, 0xa1, 0xb2, 0xc3, 0xd4, 0xe5, 0xf6};
nfc.thinkcadBeginTagEmulation(uid, 7, "ThinkCad board tag", true);
```

Der andere Sketch verwendet unverändert `readPassiveTargetID`,
`ntag2xx_ReadPage` und `ntag2xx_WritePage`. Bei `writable=false` sind entfernte
Schreibzugriffe gesperrt; der bereitstellende Sketch darf seine eigenen
Benutzerseiten weiter aktualisieren. UID-, Lock- und Capability-Seiten bleiben
auch für ihn geschützt.

Stoppen, Reset, fehlende Versorgung, getrennte Busleitungen oder das Entfernen
des Moduls beenden die Emulation. Nach einem Neustart muss der Leser den Tag neu
mit `readPassiveTargetID` auswählen. Alte Auswahlen greifen nicht auf eine neue
Emulationssitzung zu. Alle emulierten Daten bleiben auf die laufende Simulation
beschränkt. Das Modell bildet weder das rohe PN532-Tg-Protokoll noch den gesamten
ISO14443-Stack oder hardwaretypische UID-Beschränkungen bitgenau nach.

Quellsketches: [Tag bereitstellen](../examples/nfc_tag_emulation.ino) und
[Tag lesen/beschreiben](../examples/nfc_tag_peer.ino).
