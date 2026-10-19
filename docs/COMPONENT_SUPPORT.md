# Bauteilunterstützung in ThinkCad

Stand 1.3.1: Die unten genannten Modelle einschließlich der
[Analog-Erweiterung](ANALOG_SIMULATION.md).

Die Forderung nach sämtlichen Tinkercad-Bauteilen ist noch nicht vollständig umgesetzt.
Diese Liste beschreibt die tatsächlich implementierten Modelle. Sie garantiert keine
identische Hardware- oder Tinkercad-Genauigkeit.

| Bauteil | Modell und Grenzen |
| --- | --- |
| Keypad / Keyboard | 4×4- bzw. 4×8-Kontaktmatrix (25 Keyboard-Tasten), Scannen mit Pullups, optionaler serieller MIDI-Eingang; keine USB-PC-Tastaturemulation |
| Drehencoder (unveröffentlicht) | Passive A/B-Quadraturkontakte und Drucktaster; vier Flanken pro Klick, Polling-Adapter |
| Breadboard mit Stromschienen (unveröffentlicht) | 300 Signal- und 100 Schienenkontakte, vier getrennte ungeteilte Schienen; keine eingebaute Versorgung |
| Notiz (1.2.0) | Nicht elektrisch; Text per Kontextmenü/Doppelklick, verschiebbar und im Projekt gespeichert |
| Kondensator / Spule | Zustandsbehaftete Backward-Euler-Modelle; µF bzw. mH, Spule standardmäßig 0,1 Ω Wicklungswiderstand |
| NPN / PNP | Generisches Ebers-Moll-Modell mit einstellbarem β; keine Hersteller-/Temperaturparameter |
| N-/P-MOSFET | Generisches quadratisches Level-1-Modell mit Bodydiode; einstellbare Schwellspannung |
| Operationsverstärker | Generisches Verstärkermodell mit Versorgungsschienen, 50 Ω Ausgang und 50 mV Rail-Abstand; keine Bandbreiten-/Slew-Rate-Simulation |
| Masse / Verbindungspunkt / Netzlabel | Gemeinsames 0-V-Potential, explizite Abzweigungen und case-sensitive benannte Netze |
| Funktionsgenerator | Sinus, Rechteck, Dreieck, Sägezahn; 0,1–2000 Hz, Amplitude/Offset/Tastgrad |
| Oszilloskop / Logikanalysator / Stromsonde | Live-Erfassung, Trigger, Cursor, RMS/Frequenz, CSV/PNG und UART/SPI/I²C-Decoder; endliche Sample-Auflösung |
| LCD 16×2 / 20×4 | LiquidCrystal 4 Bit oder LiquidCrystal_I2C; modellierte Versorgung 2,7–5,5 V (einschließlich ESP32 3V3) und Verdrahtungsprüfung, Text/Custom-Glyphs; LiquidCrystal-API und roher PCF8574-/HD44780-I²C-Zugriff (übliche P0–P7-Belegung); paralleler Anschluss nur 4 Bit |
| AND/NAND/OR/NOR/XOR/XNOR/NOT | Versorgte generische Gatter, 1,8–5,5 V; keine Laufzeit-/Fan-out-/Gehäusemodelle |
| Servo | Servo.h und gemessene 50-Hz-Pulse; 4,5–6-V-Versorgung, Winkelanzeige; keine mechanische Last-/Getriebesimulation |
| IR-LED / 38-kHz-Empfänger / TV-Fernbedienung | Elektrische 1,2-V-IR-Diode mit Vorwiderstand; Arduino-IRremote-NEC-Adapter mit Adresse, Kommando, Repeat-Flag, decode/resume. Testfernbedienung während der Simulation mit der Maustaste gedrückt halten. Keine optische Ausbreitung oder Träger-/Pulsformsimulation |
| PN532 / NFC-Testkarte | Verdrahteter I²C-Adapter, ISO14443A-Test-UID (4/7 Bytes), synthetischer Type-2-Tag mit lesbarem NDEF-Text. Kartenpräsenz, Testgruppe und Abstand. Keine EMV-/Zahlungsdaten, MIFARE-Authentifizierung oder NFC-Rohprotokolle |
| nRF24L01+ | SPI-/CE-/CSN-Verdrahtung, 1,9–3,6 V, RF24-API, Kanal 0–125, 250/1000/2000 kbit/s, 5-Byte-Adressen, RX-Pipes, CRC-Konfiguration, ACK und 3-Paket-FIFO. Paketmodell, keine HF-/Störungs-/Registeremulation |
| 433-MHz-ASK-Sender / -Empfänger | rc-switch-Protokoll 1, 1–32 Bit, Frequenzabgleich, DATA-Pins und Versorgung. Nachrichtenmodell, keine rohe Pulsfolge; eigene Komponente je Richtung |
| DC-Motor | Ankerwiderstand/-induktivität, Gegen-EMK und Trägheit; keine integrierte H-Brücke oder Encoder |
| ESP8266 | GPIO, Pullups, 10-Bit-ADC 0–1 V; PWM über analogWrite seit 1.0.0; keine Xtensa-CPU oder WLAN |
| ESP32 / ESP32-S3 | Boardprofile mit GPIO-Auswahl, Eingangs-Pins und 12-Bit-ADC 0–3,3 V; keine CPU-/Peripherieemulation |
| Arduino Uno R3 / klassischer Nano | ATmega328P-Boardprofile: D0–D13, A0–A5, 5-V-GPIO, 10-Bit-ADC 0–5 V; Nano zusätzlich A6/A7 nur analog. PWM über analogWrite seit 1.0.0; keine AVR-CPU-, Timer- oder vollständige Peripherieemulation |
| SSD1306 | I²C, 128×64/128×32, Adafruit-GFX-Zeichenkern, Text, Bitmaps, Invertierung, Helligkeit; Scrollen mit vereinfachtem 100-ms-Zeitraster |
| Widerstand | Lineares Widerstandsmodell (DC und transient) |
| LED | Stückweise lineares Diodenmodell, 2 V Vorwärtsspannung; Überstromdiagnose |
| Taster | Idealer Schließer; kein Kontaktprellen |
| Analoge Quelle | Ideale, einstellbare Spannung mit gemeinsamer Masse |
| Potentiometer | 10 kΩ, zwei Widerstände und einstellbarer Schleifer |
| Fotowiderstand | Generische Lux-Kennlinie, 10 kΩ bei 10 lx, Gamma 0,7 |
| Thermistor | NTC 10 kΩ, B3950, 25 °C Referenz |
| Schiebeschalter | Idealer SPDT-Umschalter |
| Neigungsschalter / Reedkontakt | Einstellbarer idealer Kontakt; keine Bewegung-/Magnetfeldsimulation |
| Diode | Stückweise linear, 0,7 V und 1 Ω Durchlasswiderstand |
| 3-V-/9-V-Batterie / Netzteil | Ideale Quelle; Minuspol muss an Board-GND oder Masse liegen; keine Entladung oder Innenwiderstände |
| RGB-LED | Gemeinsame Kathode, getrennte Dioden: Rot 2 V, Grün 2,2 V, Blau 3 V |
| Siebensegmentanzeige | Gemeinsame Kathode, acht einzeln simulierte LED-Segmente |
| Voltmeter | Idealer hochohmiger Spannungsmesser |
| Piezo-Summer (passiv) | Hochohmiges 1-MΩ-Ersatzmodell; hörbare Tonwiedergabe bei `tone()` und geschlossener Verdrahtung. Gleichspannung bleibt stumm; keine Resonanz-/Kapazitätssimulation |
| Mini-Lautsprecher | Resistive 8-Ω-Last; hörbare, bandbegrenzte Rechtecksignale von `tone()`; Amplitude folgt dem Spannungsabfall. Kein Dateiplayer oder vollständiges Membranmodell |
| TMP36 | Einstellbar −40…125 °C, Ausgang 0,5 V + 0,01 V/°C, Versorgung 2,7…5,5 V. Ohne ausreichende Versorgung bleibt OUT unbeschaltet; verpolte Versorgung erzeugt einen Fehler. Idealer Ausgang, keine Einschwingzeit |
| AA-Batterie | Ideale 1,5-V-Quelle mit gemeinsamer Board-Masse |
| SPST-Ein-/Ausschalter | Rastender idealer Schließer; Ein Klick während der Simulation oder die Zustands-Checkbox öffnet/schließt den Kontakt |
| Breadboard | 170/300/630 Steckkontakte, optionale getrennte Stromschienen; a–e und f–j jeweils pro Spalte verbunden |

Release 0.5.4 enthält 25 Typen; Version 1.0.0 enthält
58 Typen einschließlich des Board-Typs. Varianten zählen nicht zusätzlich.
RGB-LED und Siebensegmentanzeige brauchen pro Kanal einen externen Vorwiderstand.
Der Arbeitsstand erweitert den Netzlöser um transiente und nichtlineare Modelle;
die Tabelle beschreibt die bisherigen Modelle. Der unveröffentlichte Arbeitsstand
ergänzt Relais, Digital-ICs und Audiobauteile; deren Status und Grenzen stehen im
[Audio- und Bauteilhandbuch](AUDIO_COMPONENTS.de.md). Beliebige I²C-Geräte und
SPI-Displays sind weiterhin nicht automatisch unterstützt. Eine Bibliotheksinstallation
ergänzt keine fehlende Hardwarephysik. Die neuen Funk-/IR-/NFC-Modelle und ihre
konkreten Anschlüsse stehen im [Modulhandbuch](WIRELESS_MODULES.de.md).

Arduino-Sketches laufen als native C++-Hostprozesse mit einer Arduino-API-Teilmenge.
Bibliotheksinstallation bedeutet daher keine vollständige ESP-Firmwarekompatibilität.
Nicht unterstützte Hardwareaufrufe sollen einen klaren Fehler statt Scheinfunktion liefern.

Mikrocontroller stehen in derselben durchsuchbaren Komponentenliste wie andere Bauteile
und können hinzugefügt oder gelöscht werden. Jedes Board hat einen eigenen Sketch;
die Board-Auswahl über dem Code-Editor wechselt zwischen den Programmen. Alle Boards
werden vor dem gemeinsamen Start kompiliert und laufen in getrennten Hostprozessen.
GPIO-Netze berücksichtigen die Treiber aller Boards. Reset, Stoppen und elektrische
Fehler stoppen bzw. starten die gesamte Simulation. Die Zeitsteuerung ist hostbasiert,
nicht zyklengenau; UART, WLAN und Multi-Master-I²C-Arbitrierung werden nicht emuliert.

Realistische Ansichten verwenden flache 2D-Formen und typische Modulabmessungen bei 6 Szeneneinheiten/mm; Arduino-Ansichten sind für bessere Lesbarkeit um 25 % vergrößert,
keine herstellerspezifischen Footprints. Gezeichnete Gehäuse und Pin-Hinweise dienen
der Orientierung; die Anschlussnamen im Projekt bestimmen die elektrische Funktion.

## Ton anschließen

- **Piezo:** `+` an Arduino D3, `−` an GND. Mit `tone(3, 440, 500)` ertönt ein
  440-Hz-Ton für 500 ms Simulationszeit. `tone(3, 880)` läuft bis `noTone(3)`.
- **8-Ω-Lautsprecher:** Arduino D3 → **470-Ω-Vorwiderstand** → `+`, `−` → GND.
  Der Vorwiderstand begrenzt den GPIO-Strom und reduziert die Lautstärke.
  Direkter Anschluss an einen GPIO überschreitet dessen Stromgrenze (40 mA
  beim Arduino, 12 mA bei ESP-Profilen) und stoppt die Simulation.
- `tone()` setzt den Pin auf OUTPUT, akzeptiert 1…20.000 Hz und kann pro Pin
  einen Ton erzeugen. `tone(pin, 0)`, `noTone()`, `digitalWrite()` oder ein neuer
  Pinmodus beenden den bisherigen Ton. Dies ist eine hostbasierte Arduino-API;
  echte Timer-Konflikte oder AVR-Timergrenzen werden nicht nachgebildet.
- Offene oder gleich verbundene Lautsprecheranschlüsse bleiben stumm. Auch
  Gleichspannung erzeugt keinen Dauerton. Mehrere Boards/Töne werden gemischt.
  Die Tonhöhe bleibt beim Ändern des Simulationstempos konstant; Notenlängen
  folgen der Simulationszeit. Stoppen, Reset und elektrische Fehler stoppen den Ton.
- Wiedergabe über das Standard-Audiogerät mit Qt Multimedia; Einstellungen →
  **Ton aktivieren** schaltet sie ab. Ohne Audiogerät läuft die Simulation weiter
  und zeigt einen Hinweis in der Ausgabe. Der neue Audio-Arbeitsstand ergänzt
  schaltungsbasierte PCM-Wiedergabe und gepufferte MP3-Dekodierung beim VS1053;
  Grenzen und noch offene Prüfungen stehen im Audio- und Bauteilhandbuch.

## TMP36 anschließen

`VCC` an Arduino 5V oder ESP 3V3, `GND` an GND, `OUT` an einen ADC-Pin.
Bei 25 °C liegen 0,75 V an OUT. Temperatur im Eigenschaftenfeld verändern.
Beim nackten ESP8266 überschreiten Temperaturen über 50 °C dessen 1-V-ADC-Bereich;
entsprechend einen Spannungsteiler oder ein anderes Board verwenden.

## Geometrie in 1.3.0

Die realistische Darstellung verwendet physische Millimeterkoordinaten und
explizite Anschlussgeometrien. Gehäusevarianten, aufgeformte Beine,
Headeradapter und die Migration älterer Direktkontakte sind in den
[Änderungen zu 1.3.0](RELEASE_NOTES_1.3.0.md) beschrieben.
Taster: vier Kontakte (1/3 und 2/4 intern verbunden).
Uno: zusätzliche obere Masse GND3. 7-Segment: COM und COM2 verbunden.

## 1.3.1: Netze, Diagnose und Messung

- Breadboard-Kontaktgruppen und die vorhandenen Stromschienen verwenden jetzt
  dieselbe explizite `NetTopology` wie der elektrische Solver. Geometrische
  Kabelkreuzungen und Überdeckungen erzeugen keine Verbindung.
- Diagnose ist regel- und zustandsbasiert. UART-Rollen stammen aus tatsächlicher
  Portkonfiguration; fehlende I²C-Bestätigung aus aufgezeichneten Transaktionen.
  Unbenutzte Eingänge werden nicht pauschal als Fehler gemeldet. Ein eigenständiger
  Batteriestromkreis braucht keinen Board-GND.
- Oszilloskop und Logikanalysator erfassen Solver-Werte mit virtueller Zeit.
  Auflösung und Genauigkeit bleiben durch den Zeitschritt und die Bauteilmodelle
  begrenzt. Pro Instrument werden bis zu 20.000 Punkte gehalten.
- UART-/I²C-/SPI-API-Aufrufe sind teilweise Transaktionsmodelle. Aus ihnen werden
  keine vermeintlich gemessenen elektrischen Busflanken erzeugt. Die Protokollansicht
  und der Simulationsinspektor zeigen diese Modelldaten ausdrücklich getrennt.
- Der Blockeditor unterstützt einen begrenzten Arduino-C++-Befehlssatz. Das
  erweitert nicht automatisch die simulierte Hardware oder Bibliotheksabdeckung.

Bedienung und genaue Grenzen: [Deutsch](VISUAL_CODING.de.md) /
[English](VISUAL_CODING.en.md).
