# ThinkCad 1.2.8

- **Hauptmenü:** übersichtliche klassische Navigation, Projekt-Suche und Sortierung; vorhandene Projektvorschauen bleiben erhalten. Einstellungen, Tutorial, Beispiele, Python und Changelog sind direkt erreichbar.
- **Bauteilliste:** Kategorienfilter, Trefferanzahl und dauerhaft einklappbare Gruppen. Die Suche öffnet passende Gruppen vorübergehend.
- **Sketch-Sprachen:** Arduino C++ und echtes MicroPython 1.26.1 lassen sich pro Board wählen und gemeinsam ausführen. GPIO, ADC, PWM, I²C, SPI, UART und virtuelle Zeit nutzen die bestehenden Schaltungsmodelle und benötigen passende Verkabelung. Python-Module werden mit dem Projekt gespeichert.
- **Lokale Python-3-Skripte:** separater Editor unter Werkzeuge mit Öffnen, Speichern, Ausführen und Stoppen. Windows enthält CPython 3.13.16; unter Linux wird Python 3 aus dem System verwendet. Ein eigener Interpreter lässt sich einstellen. Gentoo bietet dafür das USE-Flag `python`.
- **Oberflächensprachen:** Deutsch, Englisch, Russisch, Ukrainisch, Spanisch und Französisch wurden um die neuen Bedienelemente ergänzt; Italienisch und Portugiesisch kommen hinzu. Die Sprache lässt sich ohne Neustart wechseln.
- **Projektformat 6:** speichert die Sketch-Sprache pro Board; ältere Projekte werden weiterhin eingelesen. Mit Version 1.2.8 gespeicherte Projekte benötigen diese oder eine neuere Version.

MicroPython ist ein lokaler API-Emulations-Layer, keine ESP-Firmware- oder CPU-Emulation. Nicht modellierte Hardware und Module werden nicht pauschal unterstützt. Allgemeine Python-3-Skripte laufen mit den Rechten des Benutzers und greifen nicht auf die simulierten Boards zu. Details und unterstützte APIs stehen in [PYTHON.md](PYTHON.md).

ThinkCad bleibt eine lokale native C++-/Qt-Anwendung für Windows, Linux und Gentoo: ohne Login, Cloud, Electron oder macOS-Paket.
