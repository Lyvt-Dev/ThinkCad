# ThinkCad – neue Arbeitsabläufe

Dieses Handbuch beschreibt ThinkCad 1.2.0 und seine Modellgrenzen.

## Schaltungen bearbeiten

Komponenten suchen, aus der Palette auf die Arbeitsfläche ziehen und Pins verbinden. Ein Klick in der Palette platziert kein Bauteil. Strg+C/X/V kopiert, schneidet aus
und fügt ein; Strg+D dupliziert. Interne Kabel, Knicke, Eigenschaften und die
Sketches kopierter Boards werden übernommen. Externe Verbindungen werden nicht
kopiert. Gleiche Netzlabel bleiben auch nach dem Kopieren elektrisch verbunden.
Ausrichten und Verteilen stehen im Bearbeiten-Menü. R rotiert, Entf löscht,
Strg+Z/Y macht Änderungen rückgängig bzw. stellt sie wieder her.

Rechtsklick auf ein Kabel → **Abzweigung erstellen** teilt es an einem tatsächlichen
Verbindungspunkt; anschließend den neuen Zweig zu einem Zielpin ziehen. Kreuzende
Kabel sind ohne Verbindungspunkt elektrisch getrennt. Rechtsklick auf einen Pin →
**Netzlabel setzen** verbindet Pins mit identischen, groß-/kleinschreibungsabhängigen
Namen. Erlaubt: Buchstaben, Zahlen, Punkt, Unterstrich und Bindestrich, bis 48 Zeichen.
**Netz hervorheben** markiert die Kabel des Netzes. Ein ausgewähltes Netzlabel
lässt sich im Eigenschaftenfeld umbenennen; Enter übernimmt den Namen.

Einstellungen → **Raster einstellen** / **Am Raster einrasten**. Alt hebt das
Einrasten beim Verschieben kurzzeitig auf. Die Eigenschaften stehen in einem
separaten Dock neben der Komponentenpalette. Den Tab Eigenschaften zum Bearbeiten ausdrücklich auswählen. Bei Widerständen ist auch die Belastbarkeit einstellbar.

## Simulation und Diagnose

Strg+R/F5 startet oder stoppt. F6 pausiert. **Zeitschritt** integriert einen
analogen Schritt von 100 µs; Sketches laufen bis zum nächsten zeitlich
fortschreitenden delay/yield-Abschnitt. Das ist kein CPU-Instruktionsdebugger.
Ein laufender Sketch kann beim Pausieren seinen aktuellen kurzen Zeitabschnitt
noch abschließen. **Einstellungen → Für Dauer simulieren** begrenzt einen Lauf;
Sketch-Zeiten können den Zielwert um den letzten delay-Abschnitt überschreiten.

**Ansicht → Schaltungsprüfung** zeigt elektrische Fehler und Warnungen.
Aktivieren eines bauteilbezogenen Eintrags wählt das Bauteil aus. Geprüft werden
unter anderem widersprüchliche Quellen, GPIO-Überstrom, Überspannung,
Op-Amp-Versorgung/-Eingänge und Widerstandsverlustleistung. Warnungen zur
Belastbarkeit stoppen den Lauf nicht; Temperatur und Beschädigung werden nicht
simuliert. Keine Meldung bedeutet keine vollständige Hardwarefreigabe.

## Messinstrumente

Beispiele für RC, RL, Op-Amp und NPN sowie PWM sind über
Datei → Beispielschaltungen erreichbar. Der Funktionsgenerator bietet Sinus,
Rechteck, Dreieck und Sägezahn mit Amplitude, Offset und Tastgrad.

Das Messinstrumente-Dock erscheint beim Hinzufügen eines Messgeräts. Scope:
A/B gegen GND, ms/div und V/div; Stromsonde in Serie, A/div. Der Logikanalysator
hat D0…D7 und eine einstellbare Spannungsschwelle. Z bezeichnet fehlende Messwerte.
Die angezeigten Messgrößen beziehen sich auf die erfassten Samples:
Minimum, Maximum, Spitze-Spitze, RMS, bei ausreichend Flanken Frequenz und Tastgrad.

Trigger: Freilauf, Auto, Normal oder Single; steigende/fallende Flanke von A und
Pegel auswählen, mit **Bereit** neu scharf schalten. Normal wartet auf einen
Trigger; Auto zeigt ohne fertigen Trigger weiter die laufende Messung. Single
friert eine Aufnahme ein. A−B ist der zusätzliche Mathematikkanal.
Zweimal auf die Plotfläche klicken setzt Zeitcursor und zeigt Δt, 1/Δt und ΔA.
**Einfrieren**, **CSV** und **PNG** sichern bzw. exportieren die Erfassung/Ansicht.

Decoder im Logikanalysator: UART D0 (8N1, einstellbare Baudrate), SPI D0=Takt,
D1=Daten (steigende Flanke, MSB zuerst, byteweise ohne CS), I²C D0=SCL/D1=SDA
(Start/Stop, Adress-/Datenbytes und ACK/NACK). Das sind Decoder erfasster Pegel,
keine vollständige Emulation dieser MCU-Peripherie. Für zuverlässige Dekodierung
müssen ausreichend Samples pro Bit vorliegen. Maximal 20.000 Samples pro Instrument.

Die Analogmodelle sind generisch: Backward Euler, Ebers-Moll-BJT, Level-1-MOSFET
und ein begrenzter Op-Amp. Einstellungen → Analoger Zeitschritt regelt die
Auflösung. Hersteller-SPICE-Modelle, AC-Sweeps, Temperatur und CPU-Takte sind
nicht nachgebildet. Details stehen in ANALOG_SIMULATION.md.

## LCD, Logik und Aktoren

**LCD 16×2 / 20×4:** LiquidCrystal (4 Bit) und LiquidCrystal_I2C (Backpack).
Fertige Beispiele vermeiden vertauschte Anschlüsse. Parallel: VCC=5 V,
GND/RW an Masse, VO an Masse oder eine niedrige Kontrastspannung, RS/E/D4…D7
an die im Konstruktor genannten Pins. I²C: Versorgung, SDA/SCL und die Adresse
müssen passen; standardmäßig 0x27. Die Backpack-Pullups liegen an VCC.

Unterstützt werden Text/Print, clear, home, setCursor, display/noDisplay,
Cursor/Blinken, Scrollen, Schreibrichtung, Autoscroll und acht eigene 5×8-Zeichen.
I²C zusätzlich init/begin und Backlight. Die Verdrahtung wird geprüft; es handelt
sich um API-Adapter mit gerendertem Zeichenpuffer, keine taktgenaue HD44780-Bus-
oder PCF8574-Registeremulation. Nicht unterstützte rohe LCD-Kommandos melden Fehler.

**AND/NAND/OR/NOR/XOR/XNOR/NOT:** generische Gatter mit VCC/GND (1,8…5,5 V).
LOW bis 0,3×VCC, HIGH ab 0,7×VCC. Unbestimmte Eingänge erzeugen keinen gültigen
Ausgang. Keine Verzögerungszeiten, Fan-out- oder konkreten 74HC-Gehäusemodelle.

**Servo:** Servo.h, attach/write/writeMicroseconds/read/detach. SIGNAL an einen
passenden Digitalpin, Versorgung 4,5…6 V. Der angezeigte Winkel folgt gemessenen
50-Hz-Pulsen; Last, Getriebe und mechanische Trägheit des Servos sind nicht modelliert.

**DC-Motor:** Ankerwiderstand, 1 mH Standardinduktivität, Gegen-EMK und mechanischer
Anlauf. Drehzahl und Strom hängen von der Spannung ab. Ein Motor gehört an einen
Treiber, nicht direkt an GPIO. In dieser Ausbaustufe sind keine integrierten
H-Brücken, Encoder oder mechanischen Baugruppen enthalten.

## Serieller Monitor und Plotter

Ansicht → Serielle Werkzeuge. Als Quelle ein simuliertes Board oder ein echtes
Gerät wählen. Für Hardware Port und Baudrate einstellen, dann verbinden.
Für simulierte Boards sind Print/println/write und available/read/peek verfügbar.
Text senden mit LF, CRLF, CR oder ohne Zeilenende. Der Plotter erkennt Zahlenlisten
und name:wert, z. B. Spannung:3.3 Strom:0.02. Bis 16 Werte je Zeile, 2.000 Punkte.
Der Monitor hält maximal 10.000 Zeilen. Simulation und Hardware verwenden dieselbe
Ansicht; die Namen der Plotkanäle enthalten ihre Quelle.

## Echte Hardware kompilieren und hochladen

Datei → Hardware kompilieren / hochladen. Arduino CLI muss installiert sein,
ebenso der richtige Board-Core und die verwendeten Bibliotheken. FQBN ist editierbar:
Uno arduino:avr:uno; klassischer Nano arduino:avr:nano:cpu=atmega328old;
ESP8266 esp8266:esp8266:nodemcuv2; ESP32 esp32:esp32:esp32;
ESP32-S3 esp32:esp32:esp32s3. Nano-Bootloader/Boardvariante bei Bedarf anpassen.

Der komplette Sketch mit .ino-, .h- und .cpp-Dateien wird in einem temporären
Projekt gebaut. Die Simulation-SDK wird dabei nicht verwendet. Nach erfolgreichem
Kompilieren den richtigen Port wählen und Hochladen anklicken. Der Simulator und
sein serieller Port werden vorher gestoppt/geschlossen. Ausgabe und Abbruch sind
im Dialog verfügbar. Ein echtes angeschlossenes Board ist für den Upload nötig.
Linux benötigt Zugriff auf das konkrete tty-Gerät; Windows benötigt gegebenenfalls
den USB-Treiber des Boards. Die App ändert keine Geräteberechtigungen automatisch.

## Projekte, Wiederherstellung und Weitergabe

Alle 15 Sekunden werden ungespeicherte Änderungen automatisch in einer eigenen
Sitzungsdatei gesichert. Vorhandene Projektdateien werden nicht überschrieben.
Datei → Projekte wiederherstellen zeigt Sicherungen beendeter Sitzungen.
Wiederherstellen öffnet eine ungespeicherte Kopie; anschließend unter neuem Namen
speichern. Aktive Sitzungen sind durch Sperrdateien geschützt. Normales Speichern
und bewusstes Verwerfen räumen die eigene Sicherung auf. Ist der normale Datenpfad
nicht beschreibbar, wird ein temporäres Verzeichnis verwendet; auch dort hängt die
Aufbewahrung von der Systembereinigung ab. Ein Autosave ersetzt kein Backup.

Datei → **Portables Projektpaket exportieren** bettet die konfigurierten und bereits
projektgebundenen Bibliotheken mit ihren library.properties und SHA-256-Prüfsummen
ein. Empfänger benötigen diese Bibliotheksordner nicht separat. Board-Cores,
Arduino CLI und Systemwerkzeuge werden nicht eingebettet. Höchstens 20 MiB/
2.000 Bibliotheksdateien, 32 MiB Projektgröße. Dateien aus Bibliotheken müssen
weitergegeben werden dürfen. Nur vertrauenswürdige Projekte/Bibliotheken ausführen.

Projektformat 3 liest die Formate 1 und 2 weiter. Projektinformationen zeigt
Bibliotheksmetadaten und fehlende konfigurierte Verzeichnisse. Datei → Stückliste
exportiert Bauteile, Werte, Modelle und Anzahl als CSV. Das ist kein PCB-/Footprint-
Export. Eine vollständige PCB-Umgebung und Firmware-/CPU-Emulation bleiben spätere
Ausbaustufen.

## IR, NFC und Funk

Datei → Beispielschaltungen enthält NEC-TV-Fernbedienung, IR-LED-Loopback,
PN532/Testkarte, zwei ESP32 mit nRF24L01 sowie ein 433-MHz-ASK-Paar. Die Beispiele
sind vollständig verdrahtet. Ergebnisse erscheinen in Ansicht → Serielle Werkzeuge.
IR und ASK übertragen logische Nachrichten; nRF24 modelliert Paketkonfiguration,
ACK und FIFO. Die NFC-Karte ist ein synthetischer UID-/NDEF-Tag in Kartenform,
keine Zahlungskarte. Vollständige Anschlüsse, APIs und Modellgrenzen:
[IR-, NFC- und Funkmodule](WIRELESS_MODULES.de.md).

## Arbeitsfläche in 1.2

Rechtsklick auf freie Arbeitsfläche fügt eine Notiz hinzu; Doppelklick bearbeitet
sie. Notizen werden mit dem Projekt gespeichert. Nach dem ersten Speichern
werden Änderungen jede Minute in die benannte Projektdatei geschrieben
(in den Einstellungen abschaltbar). Die separate Wiederherstellung bleibt erhalten.
Während der Simulation lassen sich Bedienelemente direkt anklicken; Kabel und
statische Bauteile sind nicht auswählbar. Die Simulationsgeschwindigkeit hat
weiterhin einen Schieberegler und ein Eingabefeld für Werte von 0,1× bis 10×.
Im Startmenü öffnen Tutorials acht aufeinander aufbauende Übungen.
Einstellungen → Einstellungen öffnen bündelt Darstellung, Speichern und Simulation.

Für portable Linux-Installation und Updates siehe [AppImage-Anleitung](APPIMAGE.de.md).
