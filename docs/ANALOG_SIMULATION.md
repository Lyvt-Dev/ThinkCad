# Analoge Simulation – ThinkCad 1.0.0

Version 1.0.0 enthält die hier beschriebenen analogen Modelle und Instrumente.

## Ausprobieren

Unter **Datei → Beispielschaltungen** stehen fünf vollständig verdrahtete Aufbauten:

- **Analog: RC-Tiefpass**: Rechteckgenerator → 1 kΩ → 10 µF gegen Masse.
  Kanal A zeigt das Eingangssignal, Kanal B die Kondensatorspannung. Die Stromsonde
  misst den Ladestrom. Die Zeitkonstante beträgt 10 ms.
- **Analog: RL-Stromverlauf**: Rechteckgenerator, 100 Ω und 100 mH.
  Die Stromsonde zeigt den verzögerten Stromanstieg und -abfall.
- **Analog: Spannungsfolger**: Operationsverstärker an 5 V, OUT mit IN− verbunden,
  Sinus an IN+. Scope A/B vergleichen Eingang und Ausgang.
- **Analog: NPN-Schalter**: Basiswiderstand 10 kΩ, Kollektorlast 1 kΩ und 5-V-Quelle.
  Kanal B zeigt den invertierten Kollektorpegel.
- **Arduino: PWM und RC-Filter**: Uno D5 mit `analogWrite(5, 128)`, RC-Filter,
  Oszilloskop und Logikanalysator.

**Strg+R** startet und stoppt die Simulation. Die vier analogen Beispiele benötigen
weder Mikrocontroller noch Compiler. Ein zusätzliches Board startet weiterhin dessen
Sketch; dafür wird wie bisher ein C++-Compiler benötigt.

## Anschlüsse und Bedienung

| Bauteil | Verwendung |
| --- | --- |
| Masse | GND definiert 0 V für einen Aufbau ohne Board. Alle Massebauteile und Board-GNDs haben dieses Bezugspotential. |
| Kondensator | 1 → 2; Kapazität im Eigenschaftenfeld in µF. Startspannung 0 V. |
| Spule | 1 → 2; Induktivität in mH. Startstrom 0 A, Wicklungswiderstand standardmäßig 0,1 Ω. |
| NPN / PNP | C, B, E sind ausdrücklich beschriftet; β im Eigenschaftenfeld. Basiswiderstand vorsehen. |
| N-/P-MOSFET | D, G, S; Schwellspannung im Eigenschaftenfeld. Gate benötigt einen definierten Pegel. |
| Operationsverstärker | IN+, IN−, V+, V−, OUT. Beide Versorgungspins anschließen. Verstärkung in V/V; Ausgang hat 50 Ω und 50 mV Abstand zu den Versorgungsschienen. |
| Funktionsgenerator | OUT und GND; GND an Masse. Frequenz 0,1–2000 Hz, Sinus/Rechteck/Dreieck/Sägezahn, Amplitude als Spitzenwert (V peak), Offset und Rechteck-Tastgrad im Eigenschaftenfeld. Standard: 2,5 V Amplitude, 2,5 V Offset. |
| Oszilloskop | A und B jeweils gegen seinen GND. Hochohmige Eingänge. Eigenschaftenwert setzt den anfänglichen Zeitmaßstab. |
| Logikanalysator | D0–D7 gegen GND; einstellbare HIGH-Schwelle in V, standardmäßig 2,5 V. |
| Stromsonde | In Serie einschleifen; positives Vorzeichen von + nach −. 1 mΩ Messwiderstand; Eigenschaftenwert ist der maximale Strombereich. Überschreitung stoppt die Simulation. |

Im Dock **Ansicht → Messinstrumente** ein Messgerät auswählen. **ms/div** stellt
die sichtbare Zeitspanne ein (zehn Divisionen), **V/div** bzw. **A/div** die
vertikale Skalierung (acht Divisionen, 0 in der Mitte). **Einfrieren** hält die
Ansicht fest, während die Erfassung weiterläuft. **CSV** exportiert die aktuelle
Erfassung bzw. eingefrorene Ansicht mit Zeit in Sekunden und Spannungen/Strömen
in V/A. Pro Instrument werden maximal 20.000 Samples gespeichert. Ein fehlender
Bezug oder offener Eingang erscheint als **Z** und als Lücke; CSV-Felder bleiben
leer. Stoppen behält die Messung; Neustart/Zurücksetzen leert sie.

## PWM

```cpp
void setup() {
    analogWrite(5, 128); // Uno/Nano: ungefähr 50 %, 980 Hz
}
void loop() {
    delay(20);
}
```

Uno/Nano: D3/D9/D10/D11 mit 490 Hz, D5/D6 mit 980 Hz. Andere digitale AVR-Pins
verwenden den Arduino-Schwellwert für LOW/HIGH. Wertebereich 0–255; 0/255 ergeben
einen konstanten Pegel. ESP-Profile verwenden standardmäßig 1 kHz und 0–255;
`analogWriteRange(1…65535)` und `analogWriteFreq(1…20000)` konfigurieren folgende
Aufrufe. Änderungen bestehender PWM-Kanäle benötigen einen erneuten
`analogWrite()`-Aufruf. `digitalWrite()`, `pinMode()` und `tone()` ersetzen PWM auf
dem betreffenden Pin. Der RC-Filter verarbeitet die einzelnen Schaltphasen.
PWM wird elektrisch simuliert; die Audioausgabe bleibt auf `tone()` beschränkt.

## Rechenmodell und Grenzen

Kondensatoren und Spulen verwenden Backward Euler. Zustände bleiben zwischen
Zeitschritten erhalten; ein neuer Lauf beginnt ungeladen/stromlos. Im gestoppten
DC-Vorschaumodus ist ein Kondensator offen, eine Spule hat ihren Wicklungswiderstand.
Generische NPN/PNP verwenden ein Ebers-Moll-Modell bei festem Temperaturwert,
MOSFETs ein quadratisches Level-1-Modell einschließlich Bodydiode. Der Op-Amp ist
ein vereinfachter, versorgungsbegrenzter Spannungsverstärker. Herstellerparameter,
Sperrdurchbruch, Temperaturänderungen, Schaltladungen, Op-Amp-Bandbreite/Slew-Rate,
korrekte Op-Amp-Versorgungsströme und reale Gehäuse-Pinbelegungen sind nicht modelliert.
Dies ersetzt keine SPICE- oder Herstellerprüfung eines Hardwareentwurfs.

**Einstellungen → Analoger Zeitschritt…** begrenzt die Schrittweite auf
1–10.000 µs (Standard 100 µs). Der Generator verkleinert sie zusätzlich auf
höchstens etwa 1/64 Periode; PWM-Flanken unterteilen die Schritte ebenfalls.
Für schnelle RC/RL-Vorgänge einen deutlich kleineren Schritt als die Zeitkonstante
wählen und das Ergebnis mit halbierter Schrittweite vergleichen. Backward Euler
dämpft schnelle Schwingungen numerisch. Die kleinste Zeitauflösung beträgt 1 µs;
kurze Pulse und hochfrequente Signale können unterabgetastet werden. Die
Scope-Kurven sind zeitdiskrete Messungen, keine unendlich aufgelösten Signale.

Ein Aufruf darf höchstens 10.000 Teilschritte ausführen. Bei Überschreitung oder
fehlender elektrischer Konvergenz hält die Simulation mit einer Fehlermeldung an;
Tempo/Frequenz reduzieren oder Verdrahtung prüfen. Quellen und GPIO-Ausgänge
sind weiterhin ideal. Ein direkt an eine ideale Quelle gelegter Kondensator kann
daher einen sehr großen Anfangsstrom ergeben. Strombegrenzende Widerstände
gehören in den Aufbau. Mehrere Sketches teilen eine virtuelle Uhr auf Basis der
jeweils weitesten Sketch-Zeit; das ist keine taktgenaue MCU-Kosimulation.

## Inzwischen ergänzte Arbeitsabläufe und weitere Ausbaustufen

Version 1.0.0 ergänzt LCDs mit LiquidCrystal/4-Bit/I²C,
Logikgatter, Servo/DC-Motor, Pause/Zeitschritt, Trigger/Cursor/Decoder,
seriellen Monitor/Plotter und Kompilieren/Upload über Arduino CLI.
Das [Handbuch](V1_GUIDE.de.md) und die [Screenshot-Galerie](V1_REVIEW.md)
beschreiben den aktuellen Funktionsumfang und seine Grenzen.

Noch offen sind unter anderem vollständige Timer/Interrupts/UART/SPI/I²C-
Peripherieemulation und CPU-/Firmwareemulation, Breakpoints/Variableninspektion,
PCB-/KiCad-Werkzeuge, Encoder, Ultraschall und adressierbare LEDs.
