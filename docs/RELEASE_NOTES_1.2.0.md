# ThinkCad 1.2.0

## Arbeitsbereich

- Komponenten per Drag-and-drop platzieren; ein einfacher Klick erzeugt keine
  Komponente mehr. Beim Hinzufügen und Auswählen bleibt der gewählte Tab erhalten.
- Während einer Simulation sind Kabel und statische Bauteile nicht auswählbar.
  Klicks erreichen Taster und andere interaktive Komponenten auch unter Kabeln.
- Rechtsklick auf freie Fläche fügt Notizen hinzu. Text per Doppelklick oder
  Kontextmenü bearbeiten; Notizen bleiben im Projekt und unterstützen Undo/Redo.
- Neue Öffnen-/Speichern-Symbole und flache Simulations-/Verdrahtungswerkzeuge.
- Simulationsgeschwindigkeit über Regler oder exakte Zahleneingabe (0,1×–10×).
- Pin-Namen für weitere Bauteile ergänzt, insbesondere SSD1306 in beiden Größen.

## Lernen und Speichern

- Acht aufeinander aufbauende Lektionen im Hauptmenü: LED, Taster/Pullup, ADC,
  OLED/I²C, RC/Oszilloskop, Servo/PWM, NFC und Funk.
- Ziele werden gegen die laufende Simulation geprüft; Fortschritt bleibt lokal
  gespeichert. Eine begonnene Übung behält ihre Anleitung im Lektionen-Dock.
- Benannte Projekte werden jede Minute atomar auf der lokalen Projektdatei
  gespeichert; neue Projekte müssen vorher einmal gespeichert werden.
- Separate 15-Sekunden-Wiederherstellungspunkte bleiben erhalten.
- Neuer Einstellungsdialog mit Tabs für Darstellung, Editor/Speichern und Simulation.

## Linux und Gentoo

- Portable x86_64-AppImage mit Qt, Compiler, Beispielen und Offline-Hilfe.
- In-App-Updates mit SHA-256, GitHub-Buildnachweis, passendem Commit und atomarem
  Austausch der gestarteten AppImage. Projekte/Einstellungen bleiben erhalten.
- glibc ≥2.35; Gentoo, Ubuntu und Fedora mit passendem Desktop. Kein allgemeines
  Versprechen für ARM, 32 Bit, musl oder alte glibc-Versionen.
- Gentoo-CI kann QtSerialPort aus verifiziertem Quellcode bauen, falls der
  signierte Binärserver kein passendes Paket enthält.
- Windows-Installer, AppImage, native Linux-Pakete und Source/Overlay gehören zu
  derselben Version. Physische Mikrocontroller- und Desktop-Tests bleiben wichtig.

[AppImage-Anleitung](APPIMAGE.de.md) · [Screenshots](V12_REVIEW.md)
