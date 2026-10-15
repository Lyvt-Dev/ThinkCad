# ThinkCad 0.5.3

- Klassische Systemicons für Neu, Öffnen, Speichern und weitere Aktionen wiederhergestellt.
- Schlichte Windows-Oberfläche für Startseite und Editor; dekorative Karten und große
  Werbeüberschriften entfernt. Start/Stopp und Reset haben einheitliche Höhe und Abstand.
- Sprache und Komponenten-Design öffnen eigene Auswahlfenster mit OK/Abbrechen.
- Bauteilliste nach Mikrocontrollern, Anzeigen, Eingaben/Sensoren, Versorgung/Messung
  und Grundbauteilen gruppiert; die Suche filtert auch die Gruppen.
- Pin-Hinweise erklären Anode/Kathode, Masse, Versorgung, I²C und weitere Anschlüsse.
- Realistische Darstellung mit gemeinsamen Größenverhältnissen: 6 Szeneneinheiten pro mm,
  typische Board-, Modul- und Bauteilabmessungen, 2,54-mm-Pitch für Breadboard und OLED-Header.
  Bauteile nutzen Materialfarben, Anschlüsse und Gehäuseformen; freischwebende Bildunterschriften entfallen.
- „Über ThinkCad“ und das Tutorial beschreiben den aktuellen Funktionsumfang.
- Jedes Board besitzt einen eigenen Sketch. Die Board-Auswahl im Codebereich wechselt
  zwischen den Programmen, auch während der Simulation. Alle Sketches werden vor dem
  gemeinsamen Start kompiliert und laufen in getrennten Prozessen. GPIO, ADC und I²C-Display-
  Zuordnungen sind pro Board getrennt, die elektrischen Netze werden gemeinsam gelöst.
  Stoppen, Reset und elektrische Fehler betreffen die gesamte Simulation.
- Neues Projektformat 2 speichert die Board-Sketches. Alte Projekte bleiben lesbar;
  ihr bisheriger Sketch wird dem zuvor ausgewählten Board zugeordnet. Format 2 benötigt
  ThinkCad 0.5.3 oder neuer.

Windows und Linux einschließlich Gentoo bleiben unterstützt. Kein macOS-Support.
Die Abmessungen sind repräsentative Module, keine herstellerspezifischen Footprints.
Die Simulation bleibt eine Host-API-Simulation mit Betriebssystem-Zeitsteuerung,
keine zyklengenaue AVR-/Xtensa-Emulation oder vollständige SPICE-Analyse.
WLAN, PWM und UART-Peripherie bleiben unimplementiert.
