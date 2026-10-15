# Bauteilunterstützung in ThinkCad 0.5.0

Die Forderung nach sämtlichen Tinkercad-Bauteilen ist noch nicht vollständig umgesetzt.
Diese Liste beschreibt die tatsächlich implementierten Modelle. Sie garantiert keine
identische Hardware- oder Tinkercad-Genauigkeit.

| Bauteil | Modell und Grenzen |
| --- | --- |
| ESP8266 | GPIO, Pullups, 10-Bit-ADC 0–1 V; keine Xtensa-CPU, WLAN oder PWM |
| ESP32 / ESP32-S3 | Boardprofile mit GPIO-Auswahl, Eingangs-Pins und 12-Bit-ADC 0–3,3 V; keine CPU-/Peripherieemulation |
| Arduino Uno R3 / klassischer Nano | ATmega328P-Boardprofile: D0–D13, A0–A5, 5-V-GPIO, 10-Bit-ADC 0–5 V; Nano zusätzlich A6/A7 nur analog. Keine AVR-CPU-, PWM-, Timer- oder vollständige Peripherieemulation |
| SSD1306 | I²C, 128×64/128×32, Adafruit-GFX-Zeichenkern, Text, Bitmaps, Invertierung, Helligkeit; Scrollen mit vereinfachtem 100-ms-Zeitraster |
| Widerstand | Lineares Gleichstrommodell |
| LED | Stückweise lineares Diodenmodell, 2 V Vorwärtsspannung; Überstromdiagnose |
| Taster | Idealer Schließer; kein Kontaktprellen |
| Analoge Quelle | Ideale, einstellbare Spannung mit gemeinsamer Masse |
| Potentiometer | 10 kΩ, zwei Widerstände und einstellbarer Schleifer |
| Fotowiderstand | Generische Lux-Kennlinie, 10 kΩ bei 10 lx, Gamma 0,7 |
| Thermistor | NTC 10 kΩ, B3950, 25 °C Referenz |
| Schiebeschalter | Idealer SPDT-Umschalter |
| Neigungsschalter / Reedkontakt | Einstellbarer idealer Kontakt; keine Bewegung-/Magnetfeldsimulation |
| Diode | Stückweise linear, 0,7 V und 1 Ω Durchlasswiderstand |
| 3-V-/9-V-Batterie / Netzteil | Ideale Quelle; Minuspol muss an Board-GND liegen; keine Entladung oder Innenwiderstände |
| RGB-LED | Gemeinsame Kathode, getrennte Dioden: Rot 2 V, Grün 2,2 V, Blau 3 V |
| Siebensegmentanzeige | Gemeinsame Kathode, acht einzeln simulierte LED-Segmente |
| Voltmeter | Idealer hochohmiger Spannungsmesser |
| Breadboard | 300 Kontakte; a–e und f–j jeweils pro Spalte verbunden; keine Stromschienen |

Das sind 20 Typen einschließlich des Board-Typs; Board- und OLED-Varianten zählen
nicht nochmals als eigene Typen. RGB-LED und Siebensegmentanzeige brauchen pro Kanal
einen externen Vorwiderstand. Der Netzlöser behandelt resistive Gleichstromnetze und
vereinfachte Dioden. Kapazitive/induktive Übergänge, Transistoren, Motoren, Servos,
Relais, komplexe ICs und weitere Sensoren sind noch nicht modelliert. Auch LCD1602,
SPI-Displays und beliebige I²C-Geräte fehlen. Installation einer Bibliothek ergänzt
keine fehlende Hardwarephysik.

Arduino-Sketches laufen als native C++-Hostprozesse mit einer Arduino-API-Teilmenge.
Bibliotheksinstallation bedeutet daher keine vollständige ESP-Firmwarekompatibilität.
Nicht unterstützte Hardwareaufrufe sollen einen klaren Fehler statt Scheinfunktion liefern.

Mikrocontroller stehen in derselben durchsuchbaren Komponentenliste wie andere Bauteile
und können hinzugefügt oder gelöscht werden. Bei mehreren Boards bestimmt
„Dieses Board simulieren“ im Inspektor das Board für den gemeinsamen Sketch; die
anderen Boards treiben keine GPIO-Ausgänge. Die Auswahl wird im Projekt gespeichert.
