# ThinkCad 0.5.1

## Neue Funktionen

- Strg+R startet oder stoppt die Simulation; F5 bleibt ebenfalls verfügbar.
- R dreht ausgewählte Bauteile im Arbeitsbereich jeweils um 90° im Uhrzeigersinn.
  Mehrfachauswahl, Rückgängig/Wiederholen und das Speichern der Drehung im Projekt
  werden unterstützt. Kabelenden folgen den gedrehten Pins, manuelle Kabelknicke
  behalten ihre Position. Im Sketch-Editor und in Suchfeldern bleibt R eine normale
  Texteingabe. Während der Simulation kann die Schaltung nicht gedreht werden.

- Eine gemeinsame Komponentensuche ersetzt die Mikrocontroller-Auswahlliste.
  ESP-Boards und Arduino-Boards lassen sich wie andere Bauteile hinzufügen,
  verschieben und löschen.
- Arduino Uno R3 und klassischer Arduino Nano (ATmega328P) mit 5-V-GPIO,
  D0–D13 und 10-Bit-ADC mit 5-V-Referenz. Der Uno bietet A0–A5, der Nano
  zusätzlich A6/A7 als reine Analogeingänge. `LED_BUILTIN`, `Wire.begin()` und
  analoge Kanalnummern verwenden die passenden Boardprofile.
- Bei mehreren Mikrocontrollern wählt „Dieses Board simulieren“ im Inspektor
  das Board für den Sketch. Diese Auswahl wird im Projekt gespeichert.
- Einstellungen → Sprache bietet Deutsch und Englisch. Der Wechsel erfolgt
  sofort; die Auswahl bleibt beim nächsten Programmstart erhalten.
- Die deutsche README erklärt die Gentoo-Installation einschließlich
  Abhängigkeiten, Overlay-Registrierung, Prüfsummenprüfung und `emerge`-Befehlen.

## Korrekturen

- Kabel verlaufen geradlinig zwischen Startpin, manuell gesetzten Knicken und
  Zielpin. Zusätzliche Klicks setzen Knicke; diagonale Segmente sind möglich.
  Automatische rechtwinklige Umwege entfallen auch in der Vorschau.
- Kabelknicke bleiben verschiebbar und werden mit dem Projekt gespeichert.
- Schaltungen ohne Mikrocontroller lassen sich bearbeiten und speichern.
  Beim Löschen eines Boards werden seine angeschlossenen Kabel entfernt.
- Arduino-Eingangspegel und Ausgangsspannungen berücksichtigen das 5-V-Profil.

## Plattformen und Grenzen

Windows und Linux bleiben unterstützt. Die Linux-Pakete umfassen DEB, RPM,
Tarball, Quellarchiv und das zur Version passende Gentoo-Overlay für `~amd64`.
Die Gentoo-Anleitung steht in README.md und docs/LINUX.md.

Die Boards verwenden weiterhin eine native Host-API-Simulation. Vollständige
AVR-/Xtensa-CPU-Emulation, PWM und weitere bisher nicht modellierte Peripherie
sind nicht enthalten. Jeweils ein Board führt den gemeinsamen Sketch aus.
