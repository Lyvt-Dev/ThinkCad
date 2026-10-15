# ThinkerCad 0.3.0

- Live-Regler für Simulationsgeschwindigkeit von 0,1× bis 10× und Rücksetzung auf 1×. Arduino-Zeit und I²C-Timing bleiben in virtueller Zeit unverändert.
- Flash-Animationen können `PROGMEM` behalten. Simulatoradapter für `pgmspace.h`, `avr/pgmspace.h`, `pgm_read_byte/word/dword/float/ptr`, gängige Near-Reads, Flash-String-/Kopierfunktionen, `F` und `PSTR`.
- `THINKERCAD_SIMULATION=1` erlaubt gezieltes Ausklammern weiterer Hardwareblöcke beim Simulieren, ohne den Export für den ESP zu verändern.
- Sechs zusätzliche, fertig verkabelte Schaltungen mit passenden Sketches: LED-Blinken, Taster/LED, Ampel, Lauflicht, analoger Schwellwert und zwei OLEDs am I²C-Bus. Der Katalog enthält zusammen mit der bisherigen OLED-Animation sieben Beispiele.
- Farbige LEDs für Ampel und Lauflicht; Farben bleiben in Projektdateien erhalten. Gemeinsames vereinfachtes elektrisches LED-Modell mit 2 V Vorwärtsspannung.
- Neue Tests für Flash-Daten, Speicherzugriffe, laufende Tempowechsel und sämtliche Beispielschaltungen.

## Installation

`ThinkerCad-Setup-0.3.0.exe` ausführen, um die vorhandene Installation zu aktualisieren. Sketch-Compiler und Qt sind enthalten. Kein Administratorzugriff erforderlich.

## Grenzen

Arduino-API-Teilmenge, keine vollständige ESP8266-/Xtensa-, WLAN-, PWM- oder Flash-Controller-/Dateisystem-Emulation. Native Sketches nur aus vertrauenswürdigen Quellen ausführen. Rechenzeit reiner CPU-Schleifen wird nicht als ESP-Takt emuliert; das IPC-Ratenlimit gilt auch bei erhöhtem Tempo.

Der Entwicklungsinstaller ist ohne Signierkonfiguration unsigniert. Automatische Update-Installation bleibt bis zur Einrichtung eines vertrauenswürdigen Signierzertifikats deaktiviert.
