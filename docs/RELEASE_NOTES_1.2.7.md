# ThinkCad 1.2.7

Kleines Fehlerkorrektur-Update für die lokale, native Windows-/Linux-Anwendung.

- **Updater:** fragt ausschließlich den Commit-SHA ab. Große Commit-Antworten
  überschreiten dadurch nicht mehr die Metadatengrenze. SHA-256, signierter
  GitHub-Buildnachweis und die Bindung an den Release-Commit bleiben verpflichtend.
  Die Download-Meldung erscheint erst beim tatsächlichen Installer-Download.
- **Hilfe → Changelog:** zeigt die mitgelieferten Release Notes offline an;
  ältere mitgelieferte Versionen lassen sich auswählen.
- **Hauptmenü → Einstellungen:** öffnet direkt den vollständigen Einstellungsdialog
  mit allen sieben Kategorien, genauso wie im Editor.
- **Batterien:** AA-, 3-V- und 9-V-Batterien sowie die Gleichspannungsquelle
  funktionieren ohne direkte Verbindung zum Board-GND. Geschlossene eigenständige
  Stromkreise, Reihenschaltungen und eine verschobene Bezugsspannung werden als
  ideale Spannungsquellen berechnet. Widersprüchliche Quellen und Kurzschlüsse
  werden weiterhin gemeldet. LEDs benötigen einen passenden Vorwiderstand;
  eine einzelne AA-Zelle reicht für eine normale sichtbare LED nicht aus.

Kein Login, kein Cloud-Dienst, kein Electron und kein macOS-Paket.
