# ThinkCad 0.5.4

Aufbauend auf 0.5.3 mit separaten Board-Sketches und den aktuellen UI-Korrekturen.

- Passiver Piezo-Summer und 8-Ω-Mini-Lautsprecher mit echter Audioausgabe.
  `tone()`/`noTone()` unterstützen Frequenz, optionale Dauer, Abbruch und mehrere
  Board-Signale. Verdrahtung, Vorwiderstände und GPIO-Stromgrenzen werden berücksichtigt.
- TMP36-Temperatursensor, ideale 1,5-V-AA-Batterie und SPST-Ein-/Ausschalter.
- Flache 2D-Gehäusefarben statt Glanzverläufen; Arduino-Ansichten um 25 % vergrößert.
  Pinbeschriftungen passen in getrennte Bereiche; LED-Anode mit längerem, geknicktem Bein.
- Ton lässt sich im Einstellungsmenü abschalten. Ohne Audiogerät läuft die Simulation weiter.
- Qt Multimedia zu Linux-, Gentoo- und Windows-Builds hinzugefügt.

Modellgrenzen und Verdrahtungsbeispiele stehen in COMPONENT_SUPPORT.md.
