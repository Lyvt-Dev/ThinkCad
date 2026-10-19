# Quack-Assistent / Quack assistant

[Ergänzungsplan für 1.3.5: kontextspezifische Modellhinweise und Nyra-Skin](QUACK_ASSISTANT_PLAN_1.3.5.md)

## Bedienung

Der Assistent erklärt vorhandene Diagnosen und bietet kurze Orientierungshilfen.
Er bleibt lokal und regelbasiert. Seine Hinweise sind keine Prüfung echter Hardware.

Unter **Einstellungen → Allgemein → Quack-Assistent** und direkt im Diagnose-Dock:

| Modus | Verhalten |
| --- | --- |
| Aus | Keine Assistentenhinweise. Die normale Diagnoseliste bleibt verfügbar. |
| Nur Fehler | Fehler und Warnungen mit Handlungsbedarf, einschließlich Compilerwarnungen. |
| Normal | Zusätzlich kurze Hinweise beim Projektöffnen, Hinzufügen, Verdrahten und Vorbereiten der Simulation. |
| Ausführlich | Zusätzlich Modellgrenzen beim Starten der Simulation. |

Die Auswahl bleibt nach einem Neustart erhalten. Ein zuvor dauerhaft ausgeblendeter
Assistent bleibt beim Umstieg ausgeschaltet. **Hilfe → Quack-Assistent** oder die
Schaltfläche in der Statusleiste öffnet den Diagnose-Dock, ändert aber nicht den Modus.

Unter **Einstellungen → Allgemein → Assistenten-Skin** stehen **Quack** und
**Nyra Quack** mit Bildvorschau zur Auswahl. Erst **Speichern** übernimmt den Skin;
**Abbrechen** verwirft die Auswahl. Sie bleibt nach einem Neustart erhalten.
Nyra Quack verwendet eine transparente, aus der Nutzervorlage vorbereitete Grafik.
Beide Skins verwenden dieselben Hinweise und dieselbe Diagnose. Ein Skinwechsel
schaltet einen deaktivierten Assistenten nicht ein. Unbekannte Skin-Einstellungen
fallen auf Quack zurück. Die Grafiken sind lokal eingebettet; es gibt keinen Download.

**Hinweis ausblenden** schließt die aktuelle Karte. Dieselbe Meldung erscheint bei
Aktualisierungen nicht sofort wieder. Nach behobener und erneut auftretender Ursache
kann eine Fehlerhilfe wieder erscheinen. Orientierungshilfen erscheinen einmal pro
Kontext und Bauteil während einer Projektöffnung; beim Öffnen eines anderen Projekts
werden sie zurückgesetzt. „Aus“ deaktiviert sie dauerhaft.

Hinweise öffnen keine Fenster, übernehmen keinen Tastaturfokus und unterbrechen die
Simulation nicht. Die Statusleiste kennzeichnet einen verfügbaren Hinweis. Erst ein
Klick öffnet den Dock. **Stelle zeigen**, **Pin-Inspektor öffnen** und **Ausgabe öffnen**
werden ebenfalls nur auf ausdrücklichen Klick ausgeführt.

## Beispiel: vorhandenes OLED-Projekt prüfen

1. Das mitgelieferte verdrahtete Beispiel `animation` öffnen.
2. Den Assistenten über Hilfe öffnen und „Normal“ wählen.
3. Einen Anschluss der OLED-Versorgung entfernen. Nach Abschluss der Bearbeitung
   erklärt die vorhandene Versorgungsdiagnose das Problem und schlägt eine Prüfung vor.
4. Mit **Pin-Inspektor öffnen** die Pins des betroffenen Bauteils ansehen; Versorgung
   und Netzstatus prüfen. Den Anschluss entsprechend dem Beispiel wiederherstellen.
5. Die Simulation starten. Mit „Ausführlich“ wird zusätzlich erklärt, dass die Modelle
   nicht jede Eigenschaft echter Hardware abbilden.
6. Einen Syntaxfehler in den Sketch einfügen und erneut starten. **Ausgabe öffnen**
   führt zum Compilerprotokoll mit den ursprünglichen Datei- und Zeilenangaben.

## Umsetzung und Grenzen

- Orientierung: Projekt geöffnet, Board/Bauteil hinzugefügt, neue Verbindung
  abgeschlossen. Änderungen werden 350 ms gebündelt; während einer begonnenen
  Kabelverbindung wird die automatische Hinweisanzeige nicht aktualisiert.
- Fehlerhilfe: vorhandene elektrische Diagnosen und Solvermeldungen. Die Hilfe
  übernimmt deren Fakten und Lösungen. Fehler stehen vor Warnungen.
- Compiler: ein strukturiertes Signal meldet fehlgeschlagene Kompilierungen bzw.
  Warnungen aus dem Compilerkanal. Normale Sketch-Ausgaben sind keine Compilerwarnungen.
  Die Warnungserkennung unterstützt GCCs englische und deutsche Warnungsmarkierungen;
  das vollständige Compilerprotokoll bleibt unabhängig davon verfügbar.
- Hintergrund: Nach Dokumentänderungen und beim Simulationsstart werden Diagnosen
  auch für einen geschlossenen Dock aktualisiert. Dafür wird kein zusätzlicher
  Simulationsschritt und keine neue Netzberechnung pro Hinweis eingeführt.
- Der Assistent verändert weder Schaltung noch Sketch, Projektformat oder Solver.
  Ein erkannter Fehler ist keine vollständige Hardwareprüfung; eine ausbleibende
  Diagnose garantiert keine korrekte reale Schaltung.

In 1.3.5 sind A4988/ULN2003, Schrittmotoren, Relais, TFTs, BME680, MH-Z19B
und Share-Code angebunden. Orientierung nennt passende Pins und Modellgrenzen.
Eine tatsächliche A4988-Versorgungsdiagnose benötigt einen verbundenen Motor und
ungültige Versorgung im elektrischen Modell; aktivierte Sensorfehler werden als
simulierte Fehler bezeichnet. Unverschlüsselter Share-Export meldet fehlende
Vertraulichkeit. Die dauerhafte Sicherheitsinformation im Exportdialog bleibt
auch bei ausgeschaltetem Assistenten sichtbar.

## Tests

Die vorhandene Qt-Suite `feature129` prüft Inhalt und Zielnavigation, Migration und
Speicherung der Modi, einmalige Hinweise, Ausblenden, Fehlerpriorität, Fokus, geschlossene
Docks, den Einstellungsdialog und das Abwarten fertiger Kabelverbindungen. Echte
Kompilierungen prüfen Warnungs- und Fehlersignale. `feature128` prüft außerdem die
Vollständigkeit der Übersetzungskataloge. `desktop_smoke` startet das verdrahtete
OLED-Beispiel und prüft die simulierte Ausgabe.

## English

Choose **Quack** or **Nyra Quack** under **Settings → General → Assistant skin**.
Save applies and persists the choice; Cancel discards it. Both skins use the same
guidance. Changing the skin never enables a disabled assistant. Images are bundled
locally; unknown skin settings fall back to Quack.

The local, rule-based assistant stays inside the diagnostics dock. Choose **Off /
Errors only / Normal / Detailed** in Settings → General or in the dock. Errors only
also includes actionable warnings. The choice persists; an assistant previously
hidden in older versions stays off. Opening Help never overrides Off.

Normal adds once-per-context guidance for project opening, component placement,
completed connections and simulation preparation. Detailed also explains model limits when starting a simulation.
Hints never open docks automatically, take focus or interrupt simulation. Use the status
bar button to open a pending hint. Hide the current hint, or select Off to disable all
assistant hints. Ordinary diagnostics remain available in every mode.

Error hints suggest a practical next step and offer explicit navigation to the affected
part, its pin inspector or the compiler output. Compiler output retains original file
and line information. Hardware statements come from existing diagnostics; missing
models are not presented as supported hardware. The 1.3.5 model-specific and share-code hints use the actual implemented models
and the export encryption state.
