# ThinkCad 0.5.2

## Startseite und Bedienung

- ThinkCad öffnet ohne Dateiparameter zuerst eine eigene Startseite mit Seitennavigation,
  Projektaktionen, zuletzt geöffneten Projekten, Einstellungen, Tutorial und Beispielen.
  Ein Dateiparameter öffnet das Projekt weiterhin direkt im Editor.
- Bis zu zwölf erfolgreich geöffnete oder gespeicherte Projekte und Arduino-Sketches
  bleiben lokal in der Liste. Fehlende Dateien werden gekennzeichnet; Einträge lassen
  sich entfernen, ohne Dateien zu löschen.
- Dark Mode und Deutsch/Englisch sind auch auf der Startseite erreichbar.
- Die Startseiten-Schaltfläche in der Werkzeugleiste führt aus dem Editor zurück.
  „Zum Editor“ stellt die vorhandene Schaltung, ungespeicherten Code und die
  Sichtbarkeit der Editorfenster wieder her. Beim Verlassen des Editors stoppt die Simulation.
- Neue, eigene farbige Symbole für Neues Projekt, Öffnen und Speichern ersetzen die
  alten Systemicons. Die Startseite nutzt denselben Symbolstil.
- Start/Stopp und Reset stehen als gleich hohe Schaltflächen mit Icon und Text
  nebeneinander. Reset hat eine eigene Umrandung und bleibt in beiden Farbschemata lesbar.

## Reset korrigiert

Reset stoppt den bisherigen Sketch-Prozess und setzt virtuelle Zeit, GPIO-Zustände,
OLEDs und I²C-Bindungen zurück. Eine laufende Simulation wird mit einem neuen Prozess
ab `setup()` gestartet; eine gestoppte Simulation bleibt gestoppt. Schaltung,
Verkabelung, Sketch und Temporegler bleiben erhalten.

## Enthaltene gemeinsame Änderungen

Die Änderungen für 0.5.0 und 0.5.1 bleiben enthalten: Komponentensuche,
Arduino Uno/Nano, mehrere platzierbare Boards mit einem aktiven Simulationsboard,
Deutsch/Englisch, gerade Kabelsegmente, Strg+R/F5 für Start/Stopp und R für Drehungen.

Windows und Linux einschließlich des Gentoo-Overlays werden weiter unterstützt.
Kein macOS-Support. Die bestehenden Grenzen der Host-API-Simulation gelten weiterhin.
Linux aktualisiert weiterhin über heruntergeladene Pakete bzw. das Gentoo-Overlay;
der automatische Download mit Installation nach Neustart ist eine Windows-Funktion.
