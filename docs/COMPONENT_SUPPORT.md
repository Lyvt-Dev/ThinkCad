# Bauteilunterstützung in ThinkCad 0.5.4

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
| Piezo-Summer (passiv) | Hochohmiges 1-MΩ-Ersatzmodell; hörbare Tonwiedergabe bei `tone()` und geschlossener Verdrahtung. Gleichspannung bleibt stumm; keine Resonanz-/Kapazitätssimulation |
| Mini-Lautsprecher | Resistive 8-Ω-Last; hörbare, bandbegrenzte Rechtecksignale von `tone()`; Amplitude folgt dem Spannungsabfall. Kein Dateiplayer oder vollständiges Membranmodell |
| TMP36 | Einstellbar −40…125 °C, Ausgang 0,5 V + 0,01 V/°C, Versorgung 2,7…5,5 V. Ohne ausreichende Versorgung bleibt OUT unbeschaltet; verpolte Versorgung erzeugt einen Fehler. Idealer Ausgang, keine Einschwingzeit |
| AA-Batterie | Ideale 1,5-V-Quelle mit gemeinsamer Board-Masse |
| SPST-Ein-/Ausschalter | Rastender idealer Schließer; Doppelklick oder Zustands-Checkbox öffnet/schließt den Kontakt |
| Breadboard | 300 Kontakte; a–e und f–j jeweils pro Spalte verbunden; keine Stromschienen |

Das sind 25 Typen einschließlich des Board-Typs; Board- und OLED-Varianten zählen
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
  und zeigt einen Hinweis in der Ausgabe. Beliebige Audiodateien, `analogWrite()`
  und aus einzelnen `digitalWrite()`-Flanken rekonstruierte Audiosignale werden
  derzeit nicht unterstützt.

## TMP36 anschließen

`VCC` an Arduino 5V oder ESP 3V3, `GND` an GND, `OUT` an einen ADC-Pin.
Bei 25 °C liegen 0,75 V an OUT. Temperatur im Eigenschaftenfeld verändern.
Beim nackten ESP8266 überschreiten Temperaturen über 50 °C dessen 1-V-ADC-Bereich;
entsprechend einen Spannungsteiler oder ein anderes Board verwenden.
