# Montage, Eingaben und MIDI (ab Version 1.2.4)

## Direkt stecken statt verkabeln

In der realistischen Ansicht verwenden die Bauteile einen gemeinsamen Maßstab
von sechs Arbeitsflächen-Einheiten pro Millimeter. LED-Anschlüsse haben 2,54 mm
Abstand und gleiche Endhöhe; die Anode ist am Knick zu erkennen. Der axiale
Widerstand hat ein vorbereitetes Anschlussraster von 12,7 mm. Die modellierten Board-Header verwenden 2,54 mm Raster. Gehäuse sind repräsentative Bauformen,
keine herstellerspezifischen Fertigungs-Footprints; die Blockansicht bleibt eine
Planungsansicht.

Ein Bauteil aus der Palette ziehen: Die halbtransparente Vorschau zeigt die
Position und mögliche Kontakte. Beim Ablegen oder Verschieben rasten nahe
Anschlüsse ein; die Position, bei der mehr Anschlüsse passen, hat Vorrang.
Direktkontakte sind echte elektrische Verbindungen und werden grün markiert,
mitgespeichert und durch Rückgängig/Wiederholen wiederhergestellt. Beim
Wegziehen werden diese Kontakte getrennt. Normale Kabel bleiben erhalten.
Rotationen und gemeinsame Verschiebungen werden berücksichtigt.

Kabel liegen oberhalb aller Bauteile, auch oberhalb des Breadboards. Anschluss-
Trefferflächen bleiben zum Verkabeln erreichbar. Das Breadboard liegt unter
montierten Bauteilen. Freie Kreuzungen von Kabeln sind weiterhin keine
Verbindungen; dafür einen Verbindungspunkt setzen.

## Breadboard mit Stromschienen

Das zusätzliche **Breadboard 400 Kontakte · Stromschienen** hat 300 Signal-
und 100 Schienenkontakte. `a`–`e` und `f`–`j` bilden pro Spalte jeweils eine
Fünfergruppe. Der Mittelkanal trennt beide Seiten; die inneren Kontaktreihen e/f liegen
7,62 mm auseinander. Innerhalb der Signalgruppen gilt das 2,54-mm-Raster.

Die vier ungeteilten Schienen heißen `T+1`–`T+25`, `T-1`–`T-25`,
`B+1`–`B+25` und `B-1`–`B-25`. Jede Schiene ist nur mit sich selbst verbunden.
Plus/Minus und oben/unten sind voneinander getrennt. Sie erzeugen keine
Spannung: Versorgung und gegebenenfalls Brücken selbst anschließen.
Das bisherige Breadboard ohne Schienen bleibt verfügbar.

## Keypad, Keyboard und Encoder

- **4×4-Keypad:** acht Anschlüsse R1–R4/C1–C4. Ein Tastendruck schließt den
  jeweiligen Zeilen-/Spaltenkontakt. Keine Spannungsquelle oder versteckte
  Datenverbindung; Zeilen mit Pullups und die Spalten nacheinander scannen.
- **25-Tasten-Keyboard:** Tasten C3–C5 in einer 4×8-Matrix, Anschlüsse
  R1–R4/C1–C8. Die ersten 25 Matrixpositionen sind belegt. Alternativ lässt sich
  im Tastenfenster ausdrücklich MIDI zum seriellen Eingang des geöffneten
  Board-Sketches einschalten. Das ist ein musikalisches Keyboard, keine
  USB-PC-Tastaturemulation.
- **Drehencoder mit Taster:** A/B/SW/GND. A/B sind passive Quadraturkontakte,
  SW ist der Drucktaster. INPUT_PULLUP verwenden. Links/rechts erzeugt vier
  Flanken pro Klick, während der Simulation in virtueller Zeit geplant; der native Encoder-Adapter liest per Polling, nicht durch
  eine emulierte Interrupt-Hardware.

Die Eingaben über **Eigenschaften → Tasten öffnen**, einen Doppelklick im
Editor oder einen Klick während der Simulation öffnen. Fenster sind frei
beweglich und nicht modal. Loslassen oder Schließen gibt gehaltene Kontakte
frei. Der MIDI-Notenkanal wird für Note Off auch dann erhalten, wenn die
MIDI-Einstellung während eines Tastendrucks geändert wurde. Gedrückte Tasten
werden beim Speichern nicht als dauerhaft gedrückt gespeichert.

Matrixkontakte enthalten keine Isolierdioden. Mehrere Tasten können deshalb wie
bei einer realen ungeschützten Matrix Ghosting oder verbundene Spalten erzeugen.
Nicht gleichzeitig widersprüchliche Ausgangspegel an solche Kontakte legen.

Potentiometer und veränderliche Quellen/Sensoren während der Simulation anklicken,
um ein frei bewegliches Wertefenster zu öffnen. So bleiben sie bedienbar, obwohl
die Schaltung nicht auswählbar ist. Das DJ-Beispiel verdrahtet den Analogregler
so, dass 0–100 % auf MIDI-CC 0–127 steigt.

## MIDI-Monitor und Übersetzung

Die seriellen Werkzeuge enthalten den Tab **MIDI** und einen umschaltbaren
Text-/Hex-Monitor. Noten, Velocity, Kanal, CC und Pitch Bend werden dekodiert;
Running Status, eingeschobene Echtzeitnachrichten und begrenzte SysEx-Nachrichten
werden verarbeitet. Maximal 1.024 Bytes pro SysEx und 1.000 angezeigte Ereignisse.

Das letzte Ereignis lässt sich transponieren, auf einen anderen Kanal legen oder
von Note On/Off in einen festen Control Change umwandeln. **Übersetzen und
senden** sendet ausdrücklich an das ausgewählte virtuelle Board oder einen
bereits verbundenen seriellen Hardwareport. Die Übersetzung ist keine automatisch
rückgekoppelte Schleife. Der Hardware-Baudratenwähler enthält 31.250 Baud.

Der eingebaute `MIDI.h`-Adapter bietet die üblichen Sende-/Leseaufrufe,
Note-/CC-/Pitch-Bend-Callbacks, Kanalfilter und MIDI Thru. Es handelt sich um
serielles MIDI. Native ALSA-/Windows-MIDI-Ports, USB-MIDI-Geräteemulation und
vollständige Kompatibilität mit jedem Transport der originalen MIDI-Bibliothek
sind nicht implementiert. Für reale Firmware müssen passende Arduino-
Bibliotheken und die notwendige MIDI-Hardware vorhanden sein.

Neue ausführbare Beispiele unter **Datei → Beispielschaltungen**:

1. **Keypad: 4×4-Matrix scannen** – Keypad.h, D2–D9, Textausgabe.
2. **MIDI: 25-Tasten-Keyboard** – elektrische Matrix an D2–D13; MIDI entsteht
   durch den Sketch. Direkte MIDI-Eingabe im Tastenfenster dafür ausgeschaltet lassen.
3. **DJ: Encoder, Fader und MIDI-Pad** – Encoder D2/D3, Drucktaster D4, Pad D5,
   Fader A0. Relative CC 16, Encoder-Taster CC 17, Fader CC 7 und Pad-Note 36.

## Piezo-Piano mit 88 Tasten

Piezo auswählen und in den Eigenschaften **Piano öffnen · 88 Tasten** anklicken.
Das separate Fenster reicht von A0 bis C8. Klicken/gedrückt halten spielt den
Ton; mit gedrückter Maustaste über andere Tasten ziehen erlaubt Glissando.
Loslassen oder Fokusverlust stoppt den Ton.

Die Anzeige enthält Notenname, MIDI-Nummer, genaue Frequenz und den gerundeten
Wert für `tone(pin, frequenz)`. Grundlage ist gleichstufige Stimmung:
`f = 440 × 2^((MIDI-Note - 69)/12)`. Beispiele: A0 = 27,5 Hz, A4 = 440 Hz,
C8 ≈ 4.186,009 Hz. Der Testton verwendet dieselbe Audioerzeugung wie der
simulierte Summer, jedoch unabhängig von der aktuellen Schaltungsverdrahtung.
Die globale Toneinstellung bleibt wirksam; ohne Audioausgabegerät bleiben
Frequenzanzeige und Simulation verfügbar.

## Umfang

Die vorhandenen IR-, NFC-, Funk-, Mikrocontroller-, Analog- und Logikmodelle
bleiben nutzbar. Diese Erweiterung ist keine Zusage, sämtliche Hacking-Boards,
DJ-Geräte oder beliebigen Bauteile vollständig zu emulieren. Verfügbare Modelle
und Grenzen stehen in [COMPONENT_SUPPORT.md](COMPONENT_SUPPORT.md) und
[LIBRARY_COMPATIBILITY.md](LIBRARY_COMPATIBILITY.md).
