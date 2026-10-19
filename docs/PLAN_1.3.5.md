# ThinkCad 1.3.5 — Implementierungsplan und Architektur

Die bestehende native Qt-Anwendung bleibt erhalten. Datenpfade: Sketch-Prozess →
vorhandenes IPC → deterministische Ereigniszeit / elektrischer Solver →
Gerätezustand → gedrosselte Qt-Darstellung. Keine CPU-Emulation, kein Dienst im Netz.

## Reihenfolge und Abhängigkeiten

1. **Assistent:** bestehende Diagnose-Prioritäten und Modi beibehalten, kompakte
   Skin-Registry `assistantskin.hpp`, Ressource Nyra Quack, gespeicherte Auswahl.
   `hintAvailable`, `targetActivated`, `inspectorRequested`, `outputRequested`
   bleiben die Schnittstellen. Erst konkrete Modelle liefern konkrete Hinweise.
2. **Aktoren:** `actuators.hpp/.cpp` registriert elektrische Module; Zweige,
   Flankenbeobachtung und Telemetrie sind vom Painter getrennt. Bestehende
   `ReactiveState::electronics` hält Position (1/16 Vollschritt), STEP-Flanke,
   Spulen, Modus und Zeit. `Stepper.h` / `AccelStepper.h` erzeugen echte vorhandene
   GPIO-/Zeitnachrichten. `TransientSimulation` erhält Zustand auch beim Zeitsprung.
3. **TFT und Sensoren:** `TftController` hält Controllerregister und RGB565-RAM;
   `ModuleDevices` verarbeitet SPI/I²C. `ColorFrame` wird geteilt und nur bei
   Änderungen materialisiert; renderTimer aktualisiert die Ansicht. BME680 nutzt
   Bosch-Kalibrierung und Konversionszeit. `UartBus` hält fragmentierte MH-Z19-
   Anfragen, verzögerte Antworten und Fehler. SDK-Adapter bleiben im Sketch-Prozess.
4. **Lokales Teilen:** `ProjectShare` ist UI-unabhängiger Codec; `sharedialog.cpp`
   übernimmt Export, Passwort und bestätigte Vorschau. Argon2id/Verschlüsselung
   laufen außerhalb des UI-Threads. Qt-Kontext zerstört abgebrochene Rückrufe.
   `qrscanner.cpp` nutzt Qt Multimedia ausschließlich lokal. Kein automatischer Import.
5. **Suche:** `ComponentFinder` verwendet dieselben Katalogeinträge wie die Palette.
   Strg+P, Fokus, Pfeile/Enter/Esc und QSettings für letzte/häufige Einträge.
   Platzierung über `CircuitCanvas::mutate`, dadurch vorhandenes Undo/Redo.
6. **Integration:** verdrahtete Beispiele, Lokalisierung, Modellgrenzen,
   installierte Lizenztexte, Versionsanhebung, Gesamttests, Hauptbranch-Abgleich.
   Veröffentlichung erst aus demselben geprüften Commit für beide Betriebssysteme.

## UI und Signale

Keine neuen Überlagerungen im Arbeitsbereich. Diagnose-Dock erklärt bestätigte
Fehler; Statusleiste zeigt verfügbare Hinweise. Einstellungen übernehmen erst bei
Speichern. Das Inspektor-Pinmodell bleibt erhalten und erhält Aktor-Telemetrie.
`documentChanged` bündelt Hinweise; Laufzeit-Eingaben bleiben vom Dokument-Undo
getrennt. Finder-Dialog meldet seine gewählte Katalog-ID an die vorhandene Platzierung.
Share-Dialog gibt erst nach Bestätigung ein `DecodedShare` zurück. Kameraframes
werden lokal dekodiert, nicht gespeichert, übertragen oder als Verlauf behalten.

## Persistenz und Migration

Projektformat bleibt **8**; `libraryReferences` ist optionale, von älteren Lesern
ignorierbare Metadaten für Namen/Versionen ohne lokale Pfade. Neue Bauteile sind normale Module mit stabilen Modell-IDs
und Parametern; Board-Sketches, Dateien, Notizen, Kabelknicke und eingebettete
Modelldefinitionen verwenden `ProjectIO`. Alte Projekte bleiben lesbar.
Keine Migration laufender Gerätezustände: Simulation startet wie bisher neu.
Voransicht des Share-Imports rollt temporär registrierte eingebettete Modelle zurück.
Import wird ein neues ungespeichertes Projekt; vorhandene Dateien bleiben unverändert.

Kompatibilitätsgrenzen: Alte ThinkCad-Versionen kennen die neuen Controller nicht.
Die SDK-Adapter sind dokumentierte Teilmengen, keine Garantie für beliebige
Arduino-Libraries, MCU-Assembler, interne Timer oder alle Herstellerfunktionen.
Fremde Sketches bleiben nativer ausführbarer Code; Share-Code ist keine Sandbox.
Ein Passwort verbirgt Daten, bestätigt aber nicht den Absender.

## Testplan

- `actuator_models`: STEP/DIR, Microstep, Versorgung/Enable, Spulen, Relais NO/NC,
  Active-low, ULN2003-Sequenz, Endschalter und Diagnoseziel.
- `actuator_sdk`: GPIO-/Zeitverhalten und positive/negative Zielposition.
- `tft_models`, `tft_sdk`: Fenster, Pixelfarben, Rotation, Scrollen, Initialisierung,
  Power, SPI/DC und simulierte Übertragungszeit.
- `environment135`: echte Bosch-Kompensation, verzögerte Messung, UART-Fragmente,
  Prüfsummenfehler, fehlende Leitung/Versorgung, ausstehende Antwort und Diagnosen.
- `project_share`: Mehrboard-Roundtrip, kein Bibliotheksinhalt, zufällige Verschlüsselung,
  falsches Passwort, manipulierte Daten, Größenlimits und QR-Erkennung.
- `feature135`: alle neuen Beispiele speichern/teilen und durch den echten
  Sketch-Compiler ausführen; Importvorschau, explizite Bestätigung und Abbruch.
- `feature129`: Assistent, Modi/Skins, Fokus, Priorität, Finder und Undo.
- `feature128`: Katalogvollständigkeit und Platzhalter aller Oberflächensprachen.
- Bestehende Build-/Compiler-/Debugger-/Simulationstests und installierter Smoke-Test;
  abschließend Windows-, Linux- und Gentoo-CI des zusammengeführten Commits.
