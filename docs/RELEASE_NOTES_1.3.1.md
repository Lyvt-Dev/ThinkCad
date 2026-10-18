# ThinkCad 1.3.1

## Visual Coding, Breadboard Intelligence & Measurement

Dieses Feature-Paket war ursprünglich für 1.2.9 geplant. Es folgt als 1.3.1 auf das inzwischen abgeschlossene Release 1.3.0 und erhält dessen Bauteilgeometrie, Kontakte und Bedienungsverbesserungen.

### Visuell programmieren

Der native Qt-Block-Editor bietet **Blöcke**, **Blöcke + C++** und **C++**. Strukturierte Variablen, Verzweigungen, Schleifen und Funktionen sowie GPIO, ADC/PWM, Zeit, Serial, Servo, I²C und OLED-Text erzeugen lesbaren Arduino-C++-Code. Kompilierung und Ausführung verwenden den vorhandenen lokalen Sketch-Compiler.

Manuelle Änderungen an C++ oder ergänzenden Quelldateien markieren die Blockansicht als nicht synchron. Ersetzen erfordert eine ausdrückliche Bestätigung; eine Sicherung des manuellen Codes bleibt wiederherstellbar. Eine allgemeine C++-Rückkonvertierung ist nicht enthalten. MicroPython bleibt separat verfügbar.

### Netze und Diagnose

Solver und Oberfläche verwenden dieselbe elektrische Topologie. Hover zeigt zusammenhängende Breadboard-Kontakte und angeschlossene Pins. Beim Verkabeln erscheinen Netzvorschauen, Warnungen vor Versorgungskonflikten und überlappenden Segmenten. Optisch kreuzende Leitungen werden dadurch nicht elektrisch verbunden.

Das Andockfenster **Diagnose** bietet Schweregrad, Erklärung, Lösung und Auswahl der betroffenen Stelle: Versorgungs- und Treiberkonflikte, LED-Strombegrenzung und Rückleitung, angeschlossene schwebende Eingänge, Geräteversorgung, I²C-Adressen/NACKs und erkennbare UART-Rollenfehler. Es wertet vorhandene Modelle und konfigurierte Schnittstellen aus, nicht beliebige Sketch-Absichten. Unsichtbare Diagnosefenster führen keine Analyse aus.

### Messen und lernen

Der begrenzte Aufzeichnungspfad unterstützt zwei analoge Kanäle, acht Logikkanäle, Triggerauswahl, Pause, Cursor und CSV. Fehlende Referenzen bleiben als fehlende Werte erkennbar. Protokolltransaktionen werden als Modellansicht gekennzeichnet und erzeugen keine erfundenen elektrischen Flanken.

Zwölf verdrahtete Starterprojekte: Blink, Taster, PWM, Servo, Potentiometer, OLED-Temperaturanzeige, DHT22, HC-SR04, WS2812B, L298N-Motor, I²C-Scanner und UART zwischen zwei Boards. Jedes enthält Lernziel, Code und erwartetes Verhalten.

### Lokal exportieren

PNG/SVG der Ansicht, BOM als CSV und Projekt-ZIP mit Projektdatei, Board-Sketches, Bibliotheksliste und Readme. Lokale Bibliotheksdateien werden nur auf ausdrückliche Auswahl beigefügt. Keine Cloud-Uploads.

### Kompatibilität und Grenzen

Projektformat **8** liest Formate 1–7; bestehende Sketches, physische Layoutrevision und LAN-Daten bleiben erhalten. Ältere Programmversionen können neue Format-8-Dateien nicht öffnen. Unbekannte Blockmetadaten bleiben erhalten und verhindern normale C++-Bearbeitung nicht.

Die Blockpalette unterstützt einen Servo und ein SSD1306-Textdisplay; normale C++-Sketches sind nicht auf diese Palette begrenzt. Der Logikanalysator dekodiert vorhandene Abtastwerte: UART 8N1 auf D0, SPI mit D0-Takt/D1-Daten an steigender Flanke ohne CS, I²C mit D0=SCL/D1=SDA. Dies ist keine vollständige Chip- oder zeitgenaue Bus-Emulation. Siehe [Modellgrenzen](COMPONENT_SUPPORT.md).

Neue UI-Texte laufen durch die acht vorhandenen Sprachkataloge; Deutsch und Englisch sind vollständig ergänzt. Native Qt-Steuerelemente, Tastaturauswahl, zugängliche Namen, Palettenfarben und skalierbare Grafiken berücksichtigen Tastaturbedienung, Dark Mode und High-DPI.

Windows und Linux bleiben die Zielplattformen; Gentoo erhält die aktualisierte Ebuild mit QtSvg-Abhängigkeit. ThinkCad bleibt lokal, nativ, ohne Login und ohne Electron.

[Anleitung Deutsch](VISUAL_CODING.de.md) · [English guide](VISUAL_CODING.en.md) · [Architektur und Testplan](development/1.3.1-architecture.md)
