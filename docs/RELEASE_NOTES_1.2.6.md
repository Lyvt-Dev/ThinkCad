# ThinkCad 1.2.6

ThinkCad bleibt eine lokale, native C++20-/Qt-Anwendung für Windows und Linux:
kein Login, kein zentraler Server, kein Electron und kein macOS-Paket.

## Neue Funktionen

- **Simulation und Emulation:** pro Projekt wählbar, mit globalem Standard für
  neue Projekte. Alte Projektformate starten weiterhin in Simulation. Emulation
  ergänzt modellierte Peripherieregister, Bus- und Konvertierungszeiten.
- **Boards und Module:** ESP32-C3/S2/C6, Pico/RP2040 und STM32F103-Basisprofil;
  BME280 über I2C/SPI, MPU6050, DHT22, HC-SR04, DS18B20, WS2812B, RC522,
  L298N und MCP3008. Versorgung, Masse, Pegel und Verdrahtung werden geprüft.
- **Bibliotheken:** ausgewählte echte Adafruit-, DallasTemperature- und MFRC522-
  Quellen mit erhaltenen Lizenzen; SDK-Adapter verbinden sie mit Gerätemodellen.
  Ungenutzte Bibliotheksquellen werden beim Sketch-Build nicht mitkompiliert.
- **Lokale Komponentenpakete:** validierte JSON-Modelle, lokale Ordner und
  ausgecheckte Git-Repositories. Benutzte Datenmodelle werden im Projekt
  eingebettet; Pakete führen keinen Plugin-Code aus.
- **UART und GPIO-Interrupts:** mehrere physisch verbundene Boards, einstellbare
  Frameformate, virtuelle Byte-Zeiten, begrenzte FIFOs und Fehlerzähler.
  Externe TX/RX-Loopback-Verbindungen funktionieren ebenfalls. GPIO-Callbacks
  unterstützen Flanken, Pegel, Interruptmaskierung und `attachInterruptArg`.
- **Quellcode-Debugger:** echtes lokales GDB, Haltepunkte pro Datei/Board,
  Einzelschritte, lokale Variablen und deutlich bezeichnete PC-CPU-Register.
  Ursprüngliche `.ino`-Dateien und Zeilennummern bleiben erhalten.
- **Simulationsinspektor:** Pinspannungen, Netzverbindungen, Signaländerungen,
  modellierte Register ohne Lese-Nebenwirkungen, UART-Zähler und begrenzte
  I2C-/SPI-/UART-Aufzeichnung mit CSV-Export. Sensorwerte sind unabhängig von
  der Canvas-Auswahl auch während der Ausführung bedienbar.
- **LAN-Zusammenarbeit:** ausdrücklich gestartete Host-/Beitritts-Sitzungen mit
  Sitzungsticket. Code, Komponenten, Verdrahtung und Kabelknicke werden live
  synchronisiert. Getrennte Änderungen werden zusammengeführt; überlappende
  Änderungen bleiben sichtbare Konflikte mit lokalen Entwürfen. Die letzten
  20 Konfliktentwürfe werden separat gespeichert. Änderungen an laufenden
  Schaltungen werden bis zum Stoppen vorgemerkt. Speicherpfade und Bibliotheken
  bleiben lokal; empfangener Code wird nicht automatisch ausgeführt.
- **Einstellungen:** gruppierte native Seiten für Allgemein, Editor/Code,
  Simulation/Emulation, Komponenten, Zusammenarbeit, Performance und Tastenkürzel.
  Schrift, Tabs, Umbruch, Bildrate, Busverlauf, LAN-Limits und Shortcuts wirken
  tatsächlich und werden lokal gespeichert.
- **Sprachen und Beispiele:** neue Werkzeugoberflächen auch auf Russisch,
  Ukrainisch, Spanisch und Französisch; aktualisiertes Tutorial. Fünf neue
  verdrahtete Beispiele für Pico/BME280, ESP32-S3/MPU6050, HC-SR04, NeoPixel
  und zwei getrennte ESP32-UART-Sketches.

- **Linux-Videoimport:** Qt 6.4 initialisiert die erste Bildkonvertierung vor dem
  Abspielen, damit langsame Wayland-/EGL-Initialisierung den Decoder nicht
  während des ersten Frames blockiert.

## Plattformen

Windows-Installer mit gebündeltem Compiler und lokalem GDB samt zugehörigen
Lizenzen/Quellpaketen. Linux-Pakete empfehlen GDB als optionales Werkzeug;
AppImage kann ein lokal installiertes GDB verwenden. Gentoo erhält zusätzlich
zum bisherigen `examples`, `test`, `X` und `wayland` das optionale USE-Flag
`debugger`. Portage verwaltet dort weiterhin Installation und Updates.

## Modellgrenzen

Die Sketches laufen auf der lokalen PC-CPU. Es gibt keine Xtensa-/ARM-/AVR-
Befehls- oder vollständige Firmware-Emulation. Interrupts werden an API- und
Zeitbarrieren zugestellt; Bus-Transaktionen sind keine CPU-genauen Bitwellen.
Hardwarebibliotheken benötigen weiterhin ein unterstütztes Gerätemodell.
RC522 verwendet synthetische Testkarten ohne Kryptografie oder Karten-Speicher.
Analoge Verluste und Versorgung werden je nach Modell angenähert; thermische
Effekte und jede reale Hardwareeigenschaft sind nicht abgebildet.
LAN-Verbindungen sind unverschlüsselt und benötigen das Sitzungsticket.

Details und Bedienung:
[Gerätematrix](development/1.2.6-device-matrix.md),
[lokale Komponenten](LOCAL_COMPONENTS.md),
[Debugger](SOURCE_DEBUGGER.md),
[Inspektoren](SIMULATION_INSPECTOR.md),
[LAN-Zusammenarbeit](LOCAL_COLLABORATION.md).
