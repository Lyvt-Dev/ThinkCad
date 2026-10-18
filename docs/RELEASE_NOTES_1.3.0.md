# ThinkCad 1.3.0

## Montage und Darstellung

Die realistische Ansicht verwendet weiterhin sechs Szeneeinheiten pro Millimeter.
Diskrete Bauteile, Anzeigen, Funkmodule, Batterien und Bedienelemente werden neu
in Millimeterkoordinaten gezeichnet. Schrift und Kreise werden nicht durch
getrennte X-/Y-Skalierung verzerrt. Board-Grafiken verwenden ebenfalls physische
Koordinaten. Die Blockansicht bleibt eine schematische Planungsansicht.

Die Anschlussgeometrie ist vom Gehäuse getrennt: Beispielsweise haben die
5-mm-LED und der radiale Kondensator 2,54 mm Anschlussabstand; der axiale
¼-W-Widerstand verwendet 12,7 mm. TO-92-Transistoren und der TMP36 verwenden
auf 2,54 mm aufgeformte Beine. DIP-Adapter verwenden 2,54 mm Reihenraster
und 7,62 mm Reihenabstand. Taster verwenden ein 6×6-mm-Gehäuse mit auf
7,62×5,08 mm aufgeformten Kontakten. Beide Breadboards haben 2,54 mm Raster
und 7,62 mm Abstand über dem Mittelkanal.

Nano, NodeMCU/LoLin V3, ESP32-DevKitC und ESP32-S3-DevKitC haben definierte
Headerpositionen; GPIO- und ADC-Aliasnamen teilen gegebenenfalls denselben
physischen Kontakt. Ein Uno besitzt Buchsenleisten und wird nicht wie ein
DIP-Bauteil direkt in ein Breadboard gesteckt. Gehäuse und Raster beschreiben
konkrete repräsentative Varianten, keine universellen Fertigungs-Footprints:
Hersteller, Klone und anders geformte Beine können andere Maße haben.
Membran-Keypad, MIDI-Keyboard und Encoder besitzen modellierte Header-/Kontakt-
adapter; Motoren, Lautsprecher und Schalter mit Lötfahnen benötigen weiterhin
geeignete Anschlüsse. Batterien und Tischmessgeräte gehören neben das Breadboard.

- Taster besitzen vier Anschlüsse. 1/3 und 2/4 sind intern immer verbunden;
  Drücken verbindet beide Seiten. Bestehende Verbindungen an 1/2 bleiben gültig.
- Der Uno besitzt zusätzlich GND3 an der oberen Digitalleiste, elektrisch mit
  den übrigen Masseanschlüssen verbunden, sowie obere SDA-/SCL-Aliase für A4/A5.
- Die 7-Segment-Anzeige hat zehn Kontakte einschließlich COM2; beide gemeinsamen
  Kathoden sind intern verbunden. Der nRF24L01-Header verwendet die Signalpositionen
  des 2×4-Anschlusses.
- Die weißen Pin-Beschriftungskästen entfallen. Hover zeigt Anschlussname und
  Funktion. Aufgedruckte Headerbeschriftungen bleiben auf Board/LCD/OLED-Modulen.

## Bedienung

Wertefelder übernehmen vollständige Eingaben statt bei jeder Ziffer die
Eigenschaften neu aufzubauen. Bearbeiten, Löschen und Ersetzen einer Zahl
behält den Fokus; bestätigte Änderungen sind rückgängig machbar.

Shift beim Zeichnen eines Kabelsegments richtet dieses exakt in 45°-Schritten
aus, bezogen auf den letzten Knick bzw. Startpin. Das funktioniert auch ohne
Raster. Ohne Shift sind weiterhin beliebige Winkel möglich. Der Zielpin bleibt
der tatsächliche elektrische Endpunkt.

Unter **Einstellungen → Darstellung → Dezente Animationen** lassen sich kurze
Hover-/Druckübergänge, Platzierungs-/Kabelüberblendungen und Kontaktmarkierungen
ausschalten. Geometrie, Hitboxen und Simulation werden dabei nicht animiert.

## Bestehende Projekte

Projekte vor dieser physischen Layoutrevision behalten ihre elektrischen Netze.
Alte unsichtbare Direktkontakte werden als sichtbare Kabel geladen, weil die
korrigierten Anschlusspositionen sonst unsichtbare Verbindungen zwischen
auseinanderliegenden Bauteilen erzeugen könnten. Neu platzieren, um wieder
echte Direktkontakte herzustellen. Neue Projekte speichern die Layoutrevision im Projektformat 7. Ältere
ThinkCad-Versionen öffnen dieses Format nicht, statt es mit falscher Geometrie
anzuzeigen.

## Prüfung

Die lokale Linux-Regression umfasst 26 CTest-Suiten. Nach Anpassung der
Prüfungen an Hover-Beschriftungen und echte Pin-Aliase bestehen alle Suiten.
Zusätzliche Montageprüfungen umfassen 26 erfolgreiche Qt-Testfälle; die
LCD-/Servo-Beispiele wurden nach der Layoutkorrektur erneut kompiliert und
simuliert. Die Desktop-Suite meldet 71 erfolgreiche Fälle und einen
plattformabhängig übersprungenen Fall.
Windows- und reale Hardware-/Audio-Prüfung sind gesonderte Schritte; dies ist
die Paketprüfung erfolgt zusätzlich über die Release-CI.

## Quellen der Board-Geometrie

- [Arduino UNO-TH Rev3e Referenz-CAD](https://content.arduino.cc/assets/UNO-TH_Rev3e-reference.zip): 68,58×53,34 mm PCB, konkrete Power-/Analog-/Digital-Headerkoordinaten und 4,064 mm Abstand zwischen den Digitalleisten.
- [Espressif ESP32-DevKitC V4 Maße](https://dl.espressif.com/dl/schematics/esp32_devkitc_v4_dimensions.pdf): 27,94×48,26 mm Träger-PCB, 25,4 mm Reihenabstand, 6,04 mm Antennenüberstand.
- [Espressif ESP32-S3-DevKitC-1 V1.1 Maße](https://dl.espressif.com/dl/schematics/esp_idf/DXF_ESP32-S3-DevKitC-1_V1.1_20220429.pdf): 25,4×62,74 mm Träger-PCB, 2,54 mm Pitch und Antennen-/USB-Überstände.

Die Renderings sind eigene vereinfachte Vektorgrafiken. Gedämpft gezeichnete, nicht anklickbare Buchsen sind unmodellierte
Hardwarekontakte. Header-/PCB-Maße sind von der elektrischen Emulation zu unterscheiden; die Anwendung bildet weiterhin
eine Arduino-API-Teilmenge und keine vollständige Chip-/Peripherieemulation ab.
