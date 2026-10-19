# Quack-Assistent und Nyra Quack: Ergänzungsplan für 1.3.5

## Umfang und vorhandener Stand

Dieser Teilplan beschreibt den Assistenten; der Gesamtplan steht in [PLAN_1.3.5.md](PLAN_1.3.5.md). Motoren, Sensoren, TFTs und
Share-Code aus dem übergeordneten 1.3.5-Konzept sind separate Funktionen und
Voraussetzungen für deren konkrete Hardware- bzw. Exporthinweise.

Basis: Hauptbranch, Commit `2c0e950`. Bereits vorhanden sind:

- lokale, regelbasierte Hinweise im Diagnose-Dock, ohne Netzwerk oder KI-Dienst;
- Aus / Nur Fehler / Normal / Ausführlich, gespeichert als `duckAssistantMode`;
- Projekt-, Board-, Bauteil-, Verdrahtungs- und Simulationskontexte;
- elektrische Diagnosen und strukturierte Compilerwarnungs-/Fehlersignale;
- Ausblenden, Priorisierung, Entdoppelung und Navigation zum Pin-Inspektor bzw. zur Ausgabe;
- eine Statusleisten-Schaltfläche, die den Dock nur nach einem Klick öffnet;
- Tests in `tests/feature129_tests.cpp` und die Bedienungsanleitung
  [QUACK_ASSISTANT.md](QUACK_ASSISTANT.md).

Diese Funktionen werden weiterverwendet. Der Plan ist keine Behauptung, dass die
Hardwaremodelle über ihre dokumentierten Grenzen hinaus unterstützt werden.
Die 1.3.5-Modelle und Share-Code sind inzwischen implementiert und angebunden.
Die Skin-Auswahl **Quack / Nyra / Nyra Quack** ist inzwischen ergänzt; siehe Bedienungsanleitung.

## 1. Persönlichkeit und Hinweis-Typen

Quack und Nyra sprechen wie ein ruhiger, hilfsbereiter Kollege. Ein Hinweis nennt
kurz den Zustand und einen nächsten Schritt. Keine Emojis, Ausrufe, Rollenspieltexte
oder wiederholte Begrüßungen. Der Skin verändert weder Inhalt noch Diagnoselogik.

| Typ | Inhalt und Priorität | Beispiel |
| --- | --- | --- |
| Fehlerhilfe | Bestätigter elektrischer oder Compilerfehler; höchste Priorität | „Der Sketch konnte nicht kompiliert werden. Öffne die Ausgabe und prüfe die erste Meldung mit Dateiname und Zeile.“ |
| Warnung | Konkreter Handlungsbedarf, ohne einen unbestätigten Defekt zu behaupten | „Prüfe im Pin-Inspektor Versorgung, Masse und Netzstatus.“ |
| Orientierung | Einmal pro Kontext und Bauteil während einer Projektöffnung | „Das Board ist eingefügt. Wähle es aus und prüfe Sketch und Pinbelegung.“ |
| Modellgrenze | Erklärung der tatsächlich unterstützten Funktion | „Diese Werte sind simuliert. Eine chemische Gasanalyse ist nicht modelliert.“ |
| Teilen/Vertraulichkeit | An den wirklichen Exportzustand gebundener Hinweis | „Der Share-Code enthält deine Schaltung und deinen Code. Ohne Passwort ist er nicht vertraulich.“ |

Für Fehler bleiben `DiagnosticIssue`, Schweregrad, Ziel und Lösung die gemeinsame
Datenquelle. Keine zweite elektrische Diagnose-Engine im Assistenten.

## 2. Kontextsensitive Trigger und Voraussetzungen

| Auslöser | Quelle / Einbindung | Bedingung und Begrenzung |
| --- | --- | --- |
| Projekt öffnen | `MainWindow` → `DiagnosticsPanel::beginProject` | Orientierung einmal je Projektöffnung; kein automatisches Öffnen des Docks. |
| Board oder Bauteil hinzufügen | bestehendes `documentEdited` nach Dokumentänderung | Nur wirklich neue IDs, keine Notizen oder Netzlabels; allgemeiner Hinweis, wenn kein Modelltext verfügbar ist. |
| Kabel fertigstellen | Dokumentänderung nach abgeschlossener Kabeloperation | Vorhandene 350-ms-Bündelung und `wireInProgress()` beibehalten. Beim Ziehen keine Hinweise wechseln. |
| Simulation starten | vorhandener `SimulationStarted`-Kontext | Kurze Startorientierung in Normal; zusätzliche Modellgrenzen in Ausführlich. Fehler haben Vorrang. Gegenüber der Basis ist Normal hier gezielt zu ergänzen. |
| Elektrische Warnung / Fehler | `DiagnosticEngine` und Solverzustände | Nur bestätigte Ursachen; nach identischem Aktualisieren oder Wegklicken keine Wiederholung. Behoben und später erneut aufgetreten darf wieder melden. |
| Compilerwarnung / Fehler | vorhandenes strukturiertes Compiler-Signal | Board zuordnen, Originalprotokoll erhalten. Normale Serial-Ausgabe niemals als Compilerwarnung interpretieren. |
| Neues Relais | registriertes Relaismodell mit Versorgung und Steuereingang | Orientierung: „Dieses Relais benötigt eine Versorgung und einen Steuerpin.“ Tatsächlicher Versorgungsfehler nur aus dem Netzmodell. |
| A4988 / ULN2003 | jeweiliges Treibermodell, Motorzuordnung und tatsächliche Versorgungszustände | A4988-Beispiel nur bei zugeordnetem Motor und fehlender Versorgung: „Der Schrittmotor ist verbunden, aber der Treiber hat noch keine Versorgung. Prüfe GND, VMOT und VDD des A4988.“ ULN2003 erhält einen eigenen, passenden Pintext. |
| ILI9341 / ST7735 | konkrete Displaydefinition und Initialisierungs-/SPI-Zustand | „Prüfe CS, DC, RESET und die SPI-Leitungen dieses TFT-Modells.“ Optionale oder intern angebundene Pins nicht pauschal als fehlend melden. Versorgung und Initialisierung getrennt diagnostizieren. |
| BME680 | tatsächlicher I²C-/Versorgungszustand und Modellumfang | Simulierte Gaswerte klar kennzeichnen; keine chemische Genauigkeit behaupten. |
| MH-Z19 | Versorgung, UART-Verbindungen und modellierter Antwort-/Prüfsummenstatus | Nur bei tatsächlichem Timeout: „Der Sensor antwortet nicht. Prüfe die Versorgung und verbinde TX mit RX.“ Ein Prüfsummenfehler bekommt eine eigene Erklärung. |
| Share-Code erzeugen | zukünftiger Exportdialog meldet seinen Verschlüsselungszustand | Vertraulichkeitshinweis nur für unverschlüsselten Export; Sicherheitsinformation zusätzlich dauerhaft im Exportdialog, unabhängig vom Assistentenmodus. |

Hardwarehinweise nach stabilen Modellkennungen auswählen, nicht nach frei änderbaren
Bauteilnamen. Fehlende Modelle erhalten keine vorgetäuschten Zustandsprüfungen.

## 3. Oberfläche und Einstellungen

- Bestehenden Diagnose-Dock weiterverwenden. Keine Sprechblasen über Schaltung,
  Code, Messinstrumenten oder laufender Simulation. Keine automatisch geöffneten Fenster.
- Eine kompakte Karte mit maximal zwei kurzen Sätzen und expliziten Aktionen:
  „Stelle zeigen“, „Pin-Inspektor öffnen“, „Ausgabe öffnen“, „Hinweis ausblenden“.
  Lange technische Einzelheiten bleiben im vorhandenen Diagnose-/Compilerprotokoll.
- Tastaturfokus bleibt am aktuellen Arbeitsort; nur ein ausdrücklicher Klick oder
  Tastaturbefehl navigiert. Umbruch und Layout auch bei kleiner Fensterbreite testen.
- Modus in Einstellungen → Allgemein und im Dock synchron halten:
  Aus = keine Assistentenhinweise; Nur Fehler = Fehler und handlungsrelevante
  Warnungen; Normal = zusätzlich Orientierung; Ausführlich = zusätzlich Modellgrenzen.
- Der normale Diagnose-Dock bleibt auch bei Aus verfügbar. Hilfe überschreibt Aus nicht.
- Einzelnes Ausblenden gilt für dieselbe aktive Meldung; Aus ist dauerhaft gespeichert.
- Neue Texte ausschließlich über `Language::text` und die bestehenden Kataloge;
  Deutsch/Englisch vollständig, weitere vorhandene Sprachen mitprüfen.

## 4. Skins: Quack und Nyra

**Nyra Quack** ist der zusätzliche Skin auf Basis des vom Nutzer gelieferten Bildes.
Quack bleibt Standard und jederzeit auswählbar. Nyra erhält dieselbe professionelle
Stimme und dieselben Hinweise. Keine zusätzlichen Animationen oder Audioeffekte.

Technische Ergänzung, bewusst klein:

- Neues `src/gui/assistantskin.hpp`: kleine feste Registry aus stabiler ID,
  Anzeigename und Qt-Ressourcenpfad. IDs `quack` und `nyra-quack`; Namen nicht als
  Persistenzschlüssel verwenden. Für diese zwei Skins kein Plugin-System.
- Aus der Nutzervorlage vorbereitete transparente Grafik als `assets/nyra-quack.png` in die vorhandene Ressourcengruppe
  `thinkcad_assistant` in `CMakeLists.txt` aufnehmen. Die Bilddatei muss tatsächlich
  vorliegen; ein Platzhalter oder eine fremde Ersatzgrafik zählt nicht als Nyra.
  Transparenz und Proportionen erhalten, keine Laufzeitdownloads.
- `DiagnosticsPanel::reloadPreferences()` lädt zusätzlich die gewählte Skin-ID
  und aktualisiert das vorhandene `duckAssistantMascot` samt zugänglichem Namen.
- `preferences.cpp`: Auswahl „Assistenten-Skin: Quack / Nyra / Nyra Quack“ mit kleiner statischer
  Vorschau. Erst Speichern übernimmt die Auswahl; Abbrechen verändert nichts.
- QSettings-Schlüssel `duckAssistantSkin`, Standard `quack`. Unbekannte IDs oder
  nicht lesbare Bildressourcen fallen zuverlässig auf Quack zurück.
- Im Dock bleibt der Bildplatz kompakt und unverändert; größere Vorschau nur in
  den Einstellungen. `KeepAspectRatio`, High-DPI-taugliche Skalierung, kein Einfluss
  auf Theme, Fenstergröße, Texte oder Simulator. Bild nur beim Skinwechsel laden.

## 5. Dateien, Signale und Reihenfolge

1. Bestehende Assistententests auf aktuellem Hauptbranch ausführen. Vorhandene
   Arbeit übernehmen, keine unabhängige Neuimplementierung.
2. Skin-Registry, Nyra-Ressource und Einstellung ergänzen:
   `assistantskin.hpp`, `CMakeLists.txt`, `preferences.cpp`, `diagnosticspanel.cpp`,
   Sprachkataloge. Bestehenden Reload nach gespeicherten Einstellungen verwenden.
3. Kurzen Start-Hinweis für Normal ergänzen; ausführliche Modellgrenzen nur in
   Ausführlich. Priorisierung, Ausblenden und normale Diagnoseliste erhalten.
4. Mit den separaten 1.3.5-Modellen deren präzise `DiagnosticIssue`-Regeln und
   Orientierungstexte ergänzen. Vorher ist dieser Teil absichtlich nicht aktiv.
5. Share-Code-Dialog nach dessen Implementierung über ein typisiertes Kontextsignal
   anbinden; keinen gesamten Share-Code, kein Passwort und keine Projektdaten an
   den Assistenten übergeben oder dafür protokollieren.
6. Tests und Dokumentation abschließen, dann in den gemeinsamen Release integrieren.
   Kein separater konkurrierender Release für den Skin.

Bestehende Signale `hintAvailable`, `targetActivated`, `inspectorRequested` und
`outputRequested` bleiben erhalten. Die eigentlichen Hardwareprüfungen gehören nach
`src/core/diagnostics.cpp`; der Assistent erklärt und verlinkt ihre Ergebnisse.

## 6. Test- und Abnahmekriterien

`tests/feature129_tests.cpp` erweitern, vorhandene Tests weiterlaufen lassen:

- Alle vier Modi, Migration alter Sichtbarkeitseinstellungen, Neustart-Persistenz,
  Einstellungen speichern/abbrechen, Hilfe bei Aus und sofortiges Einzel-Ausblenden.
- Projekt öffnen, Bauteil hinzufügen, Kabel fertigstellen und Simulation starten:
  passende kurze Hilfe, einmaliger Kontext, Fehler vor Orientierung. Normal und
  Ausführlich bei Simulationsstart gezielt unterscheiden.
- Fehler, Warnungen und Compilerhinweise nennen einen nächsten praktischen Schritt;
  Navigation wählt das richtige Bauteil/Board. Keine falsche Compilerwarnung durch Serial.
- Geschlossener Dock bleibt geschlossen, Fokus unverändert, keine Unterbrechung
  laufender Simulation und keine Aktualisierung mitten in einer Kabeloperation.
- Skinwechsel aktualisiert Bild und zugänglichen Namen; Aus bleibt Aus. Inhalte,
  Hinweispriorität und Simulation bleiben mit beiden Skins identisch.
- Beide Bildressourcen vorhanden und nicht leer; Nyra-Verhältnis bleibt erhalten.
  Fehlende/unbekannte Skin-ID fällt auf Quack zurück. Auswahl bleibt nach Neustart
  erhalten, Abbrechen des Einstellungsdialogs verwirft sie.
- Modellbezogene Hinweise jeweils mit dem verdrahteten Beispiel testen:
  korrekter Aufbau erzeugt keinen falschen Fehler; fehlende Versorgung, getrenntes
  GND, falsche Buszuordnung und tatsächlich modellierte Kommunikationsfehler schon.
  Ein umbenanntes Bauteil darf die Modellzuordnung nicht ändern.
- Share-Code: unverschlüsselt / verschlüsselt / Export abgebrochen getrennt testen;
  Sicherheitsinformation bleibt bei ausgeschaltetem Assistenten im Exportdialog.
- Übersetzungskataloge mit `feature128` prüfen. Manuell Hell/Dunkel, 100/150/200 %
  Skalierung, kleine Fensterbreite, Tastaturbedienung und Screenreader-Beschriftung prüfen.
- Große Schaltung: nur bestehende Diagnoseergebnisse und gebündelte Ereignisse
  verwenden, keine neue Netzberechnung oder PNG-Dekodierung pro Simulationstick.

Dokumentation: `QUACK_ASSISTANT.md` um Skinwahl und neue Kontextbeispiele ergänzen;
Release Notes 1.3.5 nennen Modi, Nyra und tatsächlich integrierte Modellhinweise.
Für eine Abnahme die jeweils vorhandenen verdrahteten Beispielprojekte verwenden,
keine nicht unterstützten Hardwarebeispiele als funktionsfähig ausgeben.

## 7. Kompatibilität und Modellgrenzen

Keine Änderung des `.thinkcad`-Formats, an Sketches, Solver, Undo/Redo oder lokalen
Zusammenarbeitsdaten. Skin und Hinweismodus sind lokale Einstellungen, keine
Projektbestandteile. Vorhandene `duckAssistantMode`-/Sichtbarkeitseinstellungen
bleiben gültig. Keine Cloud, kein Login, kein Electron, keine zusätzliche Laufzeitabhängigkeit.

Weder Quack noch Nyra prüfen echte Hardware. Fehlende Diagnosen garantieren keine
elektrisch oder mechanisch sichere reale Schaltung. Nicht modellierte Eigenschaften
werden als Grenzen erklärt, niemals als gemessene Tatsachen dargestellt.
