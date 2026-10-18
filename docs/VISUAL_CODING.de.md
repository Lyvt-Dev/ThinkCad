# Blöcke, Breadboard und Messungen

ThinkCad 1.2.9 bleibt eine lokale native C++20-/Qt-Anwendung. Ein geöffnetes
Projekt führt keinen Code aus. Erst **Simulation starten** kompiliert und startet.

## Mit einem fertigen Blockprogramm beginnen

Unter Beispiele die Starter-Projekte **LED blinken** oder **Taster und LED** öffnen.
Beide enthalten Verdrahtung und bearbeitbare Blöcke. Jedes Board behält seinen
eigenen Sketch, seine Sprache und seine Blockdaten.

Die Ansichten heißen **Blöcke**, **Blöcke + C++** und **C++**. Blöcke erzeugen
Arduino-C++; MicroPython bleibt separat über die Sketch-Sprache verfügbar. Während
der Simulation und für MicroPython ist die Blockbearbeitung gesperrt.

Eine Karte auswählen, um ihre Eigenschaften zu ändern. Karten lassen sich mit
Maus oder Pfeiltasten anordnen. Ihre Position verändert ausschließlich die Ansicht.
Die Ausführungsreihenfolge und Verschachtelung werden im Reiter **Struktur** durch
Verschieben in Anweisungscontainer oder Zweige festgelegt. Der Strukturbaum ist
auch per Tastatur bedienbar.

**Block hinzufügen**, Blocktyp und Argumente wählen, dann **Block übernehmen**.
Pro Argument eine Ausdruckszeile eingeben. Globale Variablen und Funktionsparameter
besitzen Tabelleneditoren. Datentypen sind `int32_t`, `double`, `bool` und `String`.
Namen bestehen aus ASCII-Buchstaben, Ziffern und Unterstrichen.

Ausdrücke unterstützen Werte, Variablen, Funktionsaufrufe, Rechenoperationen,
Vergleiche, logische Verknüpfungen und Klammern, etwa `digitalRead(12) == LOW`.
Text steht in doppelten Anführungszeichen. Freie C++-Anweisungen oder Präprozessor-
Direktiven werden nicht als Ausdruck angenommen. Die Zählschleife verwendet einen
Indexnamen sowie Anfangswert, exklusive Grenze und konstante Schrittweite ungleich null.

Befehle umfassen GPIO, ADC/PWM, delay/millis, Serial, ein Servo-Objekt, Wire-I²C
und SSD1306-Text. Initialisierung ausdrücklich in setup ergänzen. Für weitergehende
Bibliotheken oder mehrere Servo-Objekte steht normaler C++-Code zur Verfügung.

## C++ sicher bearbeiten

Der erzeugte Sketch läuft durch denselben separaten Compiler wie normale .ino-
Projekte. Es gibt keine allgemeine Rückkonvertierung von C++ in Blöcke.

Manuelle C++-Änderungen markieren die Ansichten als **nicht synchron**. Sie löschen
keine Blöcke. Blockänderungen überschreiben manuell bearbeiteten Code nicht
unbemerkt. Vor dem Ersetzen fragt die App nach und speichert eine wiederherstellbare
Sicherung in den Projektmetadaten. **Manuelle Sicherung wiederherstellen** holt sie
zurück. Es handelt sich um eine Sicherung, nicht um eine Versionsverwaltung.
Unbekannte Blockformate bleiben zusammen mit dem C++ erhalten; Entfernen erfordert
einen ausdrücklichen Reset. Abbrechen erhält den Sketch.

## Breadboard und Diagnose

Beim Hover werden alle elektrisch verbundenen Löcher, Pins und Kabel hervorgehoben.
Beim Verkabeln zeigt die Vorschau die Vereinigung der Netze. Solver und Oberfläche
verwenden dieselben Kontaktgruppen, Stromschienen, Schalter, Aliasse und Netzlabels.

Eine Kreuzung oder Überdeckung verbindet keine Kabel. Abzweigungen benötigen einen
expliziten Knoten. Doppelte Kabel werden abgewiesen; teilweise überdeckte Kabel und
Versorgungskonflikte lösen Warnungen aus. Bestätigen einer Warnung beseitigt keine
elektrische Gefahr. Kabelknicke und Undo/Redo bleiben erhalten.

**Ansicht → Diagnose** zeigt Schweregrad, Erklärung und Lösung. Enter oder Doppelklick
führt zum Anschluss beziehungsweise Bauteil. Regeln erkennen Versorgungskurzschlüsse,
Treiberkonflikte, LED-Überstrom und fehlenden Rückweg, verdrahtete schwebende Eingänge,
fehlende Modulversorgung, I²C-Konflikte und bekannte UART-Verwechslungen.

Unbenutzte, unverdrahtete GPIO-Eingänge werden nicht einzeln beanstandet. Ein
abgeschlossener Batteriestromkreis braucht keinen Board-GND. Pfadprüfungen sind
konservativ; berechnete Ströme und Spannungen stammen weiterhin aus dem Solver.
UART-Prüfungen benötigen die tatsächliche UART-Konfiguration. I²C-Zugriffsdiagnosen
verwenden aufgezeichnete Transaktionen bei geöffnetem Panel; erfolglose leere
Scanner-Probes sind Hinweise. Ein verborgenes Panel führt keine Diagnoseanalyse aus.

## Messen: elektrische Spur und Protokollansicht unterscheiden

Oszilloskop A/B und GND anschließen. Spannungen beziehen sich auf den Instrument-
GND. Der Logikanalysator erfasst D0–D7 mit der eingestellten Schaltschwelle.
Zeitbasis, Spannungsbereich, Triggerkanal/-flanke und Modus wählen. Einfrieren hält
die angezeigte Aufnahme; die begrenzte Erfassung läuft weiter. Klicks setzen
abwechselnd Cursor; angezeigt werden Δt und die Differenz von Kanal A. CSV exportiert
die angezeigte, gegebenenfalls eingefrorene Aufnahme.

- GPIO, PWM und analoge Antworten kommen aus der deterministischen Ereignis-/
  Transientensimulation mit virtueller Mikrosekundenzeit.
- Pro Instrument werden höchstens 20.000 Punkte gehalten. Überschreiben alter
  Punkte wird gemeldet. Unbekannte Spannung oder fehlender Bezug bleiben unbekannt;
  CSV enthält dafür ein leeres Feld.
- Bitdecoder: UART auf D0, 8N1 mit gewählter Baudrate; SPI D0=Takt und D1=Daten,
  steigende Flanke, MSB zuerst, ohne CS-Auswertung; I²C D0=SCL und D1=SDA.
  Das sind begrenzte Lerndecoder, keine universellen Hardwareanalysatoren.
- **Protokollansicht (Modell)** zeigt tatsächlich modellierte UART-/I²C-/SPI-
  Transaktionen. Daraus werden keine elektrischen Busflanken erfunden. Für Bytes
  und Register zusätzlich den vorhandenen Simulationsinspektor verwenden.
- Zeitauflösung, analoger Zeitschritt und Bauteilmodell begrenzen die Messung.
  Es gibt keine erfundene Sub-Mikrosekundenauflösung, reale Oszilloskopbandbreite,
  Tastkopfkapazität oder Rauschsimulation.

## Starter und Export

Zwölf frei zugängliche Starter-Projekte stehen in Beispiele und Lernpfad bereit:
Blinken, Taster, PWM, Servo, Potentiometer, OLED-Temperatur, DHT22, Ultraschall,
WS2812B, L298N-Motor, I²C-Scanner und UART zwischen zwei Boards. Jedes enthält
Lernziel, Verdrahtung, Code, erwartetes Verhalten und Aufgabe. Die ursprünglichen
acht aufeinander aufbauenden Lektionen bleiben erhalten.

**Datei → Exportieren** bietet PNG/SVG der aktuellen Ansicht, eine CSV-Bauteilliste
und ein Projekt-ZIP. Enthalten sind Projektdatei, Sketches je Board, BOM,
Bibliotheksmanifest und README. Bibliotheksnamen und Versionen nennen keine
absoluten lokalen Pfade. Bibliotheksdateien sind optional, das Simulator-SDK gehört
zur ThinkCad-Installation. ZIP verwendet Standard-Einträge mit CRC32 und UTF-8-
Namen. Alles wird ausschließlich lokal gespeichert.

Projektformat 7 liest weiterhin Formate 1–6. Blockdaten, Kartenpositionen,
Generatorversion, Fingerabdruck und Sicherung werden pro Board aufbewahrt.
Ältere ThinkCad-Versionen können Format 7 nicht lesen. Laufende Messdaten und
Topologie-Caches werden nicht eingebettet. Wiederherstellung, LAN und Board-
Kopieren erhalten die Metadaten; empfangener Code startet nicht automatisch.
