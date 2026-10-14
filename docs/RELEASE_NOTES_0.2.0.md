# ThinkerCad 0.2.0

- Neues, gut erkennbares ThinkerCad-Logo für App, Taskleiste und Installer.
- Normale Windows-Titelleiste mit Schließen, Minimieren, Maximieren und veränderbarer Größe; Startgröße passt zum Bildschirm.
- Moderne helle Oberfläche mit Bauteil-Auswahl, Code-Tabs, Zeilennummern, Syntaxfarben und Ausgabe.
- Frei verkabelbare Schaltung: ESP8266, OLED, LED, Widerstand, Taster und Analogquelle. Bauteile verschieben, Pins verbinden, Kabel löschen, Undo/Redo und Zoom.
- `.ino`-Projekte einschließlich weiterer Tabs, Header und lokaler C++-Quellen öffnen und testen.
- Schaltung und Code als portable `.thinkercad`-Datei speichern; bearbeitete Arduino-Projekte exportieren.
- Resistives DC-Modell, LED-Strom, Pullups, schwebende Eingänge und Kurzschlussdiagnosen. OLED verlangt richtige Versorgung und I²C-Verkabelung.
- Flüssigere Oberfläche durch begrenzte IPC-Verarbeitung und gebündelte Display-Updates; Schließen beendet den Sketch-Prozess.

## Windows installieren

`ThinkerCad-Setup-0.2.0.exe` herunterladen und ausführen. Die vorhandene Installation kann aktualisiert werden. Qt-Laufzeit und Sketch-Compiler sind enthalten. Kein Administratorzugriff erforderlich.

## Grenzen

Arduino-API-Teilmenge, keine vollständige ESP8266-/Xtensa-, WLAN-, PWM- oder LCD1602-Emulation. Komplexe `.ino`-Prototypfälle benötigen explizite Deklarationen. Native Sketches und Bibliotheken nur aus vertrauenswürdigen Quellen ausführen.

Dieser Entwicklungsinstaller ist ohne Signierkonfiguration unsigniert; Windows kann eine SmartScreen-Meldung anzeigen. Automatische Update-Installation bleibt bis zur Einrichtung eines vertrauenswürdigen Signierzertifikats deaktiviert.
