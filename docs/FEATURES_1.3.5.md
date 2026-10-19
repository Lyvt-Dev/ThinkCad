# ThinkCad 1.3.5 — Bedienung und Modellgrenzen / Usage and model limits

## Schrittmotoren / Steppers

Unter Beispiele stehen `stepper_a4988`, `stepper_accel`, `stepper_uln2003` und
`stepper_unipolar6`. Sie enthalten jeweils die komplette Verdrahtung und einen Sketch.

- A4988: VDD 3–5,5 V, VMOT 8–35 V, gemeinsame Masse, RESET/SLEEP HIGH,
  ENABLE LOW. STEP zählt steigende Flanken; DIR bestimmt die Richtung.
  MS1/MS2/MS3: 000 Voll-, 100 Halb-, 010 Viertel-, 110 Achtel-, 111 Sechzehntelschritt
  (hier in der Reihenfolge MS1/MS2/MS3). Andere Kombinationen sind deaktiviert.
- `AccelStepper::setPinsInverted(false,false,true)` vor `setEnablePin` benutzen,
  wenn ENABLE direkt am aktiven LOW-Eingang des A4988 liegt.
- ULN2003: IN1–4 und GND verbinden; Motor-Mittelanzapfung an 5 V.
  COM ist der gemeinsame Freilaufanschluss, keine Versorgung der Logik.
- Vierleiter bipolar, Fünfleiter 28BYJ-48, Sechsleiter mit zwei Mittelanzapfungen.
  Vollschritte/Umdrehung und Wicklungswiderstand sind einstellbar.
  28BYJ-48 ist auf nominell 2048 Vollschritte bezogen; reale Übersetzung variiert.
- Die Welle zeigt relative Position; erste Spulenausrichtung definiert Null.
  Inspektor: Position/Winkel, Schritte, Modus, Spulen, Schrittrate und rpm.
  AccelStepper-DRIVER meldet sein Sketch-Ziel zusätzlich als Telemetrie.
  Bei direktem GPIO oder anderen Treibern bleibt das Ziel unbekannt.
- Endschalter: COM/NC oder COM/NO verdrahten und im Sketch auswerten.
  Der Hebel ist während der Simulation bedienbar. Kein automatisch abgeleiteter
  mechanischer Achsweg, keine Last, Trägheit, Resonanz, Schrittverluste,
  Chopper-Stromregelung oder thermische Schutzabschaltung.

**APIs:** `Stepper` (2/4 Pins, `setSpeed`, `step`, `version`), `AccelStepper`
(DRIVER/FULL2WIRE/FULL4WIRE/HALF4WIRE, Positions-/Geschwindigkeitsziele, `run`,
`runSpeed`, `runToPosition`, `stop`, Enable/Inversion/Pulsbreite). Software-
Beschleunigung ist eine Näherung; Funktionscallbacks und 3-Wire-Modi fehlen.

**EN:** Wired examples drive actual simulated GPIO. A4988 requires valid logic and
motor supplies and active-low ENABLE; invert the enable pin in AccelStepper.
The visible shaft is relative to initial alignment. No load dynamics, current
chopping, thermal model or automatic physical axis/endstop coupling is claimed.
The AccelStepper DRIVER adapter reports a sketch target as explicit software
telemetry; direct STEP/DIR has no observable target.

## Relais / Relay

`relay_button`: Uno + Taster + Relais + LED mit 220 Ω. VCC/GND mit 5 V versorgen,
IN steuert COM→NO; im Ruhezustand COM→NC. Optional Active-low im Inspektor.
Status-LED, Kontakte und optionaler dezenter Klick folgen dem tatsächlichen Zustand.
Einstellungen → Allgemein → Relais-Klick wiedergeben; globale Audioabschaltung gilt
zusätzlich. Keine Kontaktschwingungen, Lichtbögen, Isolation oder Netzspannungsfreigabe.

**EN:** A powered 5 V relay changes COM between NC and NO, with optional active-low
input. The click is asynchronous and separately configurable; master sound also
mutes it. Contacts are a simple resistive model, not a mains-safety model.

## TFT

`tft_ili9341` / `tft_st7735`: ESP32-S3 mit MOSI11, MISO13, SCK12,
CS10, DC9, RESET8. VCC und LED an 3V3, GND verbinden. MISO ist bei reinen
Schreibzugriffen optional. Unversorgte, falsch verdrahtete oder nicht initialisierte
Displays geben kein normales Bild aus.

`Adafruit_ILI9341.h` und `Adafruit_ST7735.h` verwenden den vorhandenen GFX-Kern:
Pixel, Linien, Rechtecke, Kreise, Dreiecke, Text/Fonts/Textfarben, Bitmaps,
RGB565, Rotation. ILI9341: 240×320 / 320×240; ST7735: 128×160 / 160×128.
SPI-Bytezeit zählt zur simulierten Zeit; Übertragung in begrenzten Blöcken,
Ansicht standardmäßig 30 Hz. Registermodell: Reset/Sleep/Display on, COLMOD16,
MADCTL, Adressfenster, RAMWR, Invertierung und vertikale Scrollregister.

**EN:** Native SPI controller model with GFX drawing and RGB565 framebuffer.
Only the listed resolutions/controllers are modeled. No full analog panel,
gamma/power circuit, 18-bit color, arbitrary ST7735 variants or accurate readPixel
behavior. The protocol inspector represents transactions, not individual SPI edges.

## BME680 und MH-Z19B / Environmental sensors

`bme680_monitor`: I²C SDA8/SCL9, VCC3V3/GND, Adresse 0x76 (oder 0x77).
Temperatur, Feuchte, Druck und **simulierter** Gaswiderstand im Inspektor einstellen;
optional deterministische Schwankungen oder keine Antwort. Die originale
Adafruit/Bosch-Bibliothek liest Chip-ID, synthetische Kalibrierung und ADC-Register.
Forced-Konversion berücksichtigt Oversampling und Heizwartezeit. Kein BSEC,
VOC-Index, chemisches Gasgemisch oder Gesundheits-/Luftqualitätsversprechen.

`mhz19_monitor`: VCC5V/GND; TX des Sensors an GPIO17 (RX), RX an GPIO18 (TX).
UART 9600 8N1, 3,3-V-Logik. `MHZ19.h` unterstützt `begin`, `getCO2`,
`getTemperature`, `isValid`. Der 9-Byte-Befehl 0x86 liefert verzögert ppm und
Prüfsumme. ppm/Temperatur/Schwankung/Antwortzeit sowie keine Antwort oder falsche
Prüfsumme sind einstellbar. Kein echtes NDIR-, Aufwärm- oder Kalibriermodell.
Ausstehende Bytes werden bei Versorgungsausfall abgebrochen; bereits empfangene
FIFO-Bytes bleiben empfangen. UART-Protokollansicht ist keine analoge Leitungsform.

**EN:** All environmental/gas values are simulated inputs, with optional deterministic
variation and fault injection. BME680 supports forced measurements through actual
Bosch compensation. MH-Z19B supports delayed checksummed CO2 reads over correctly
wired UART, not calibration or physical gas chemistry.

## Bauteilsuche / Component finder

**Strg+P / Ctrl+P** öffnet den Katalogdialog. Suchfeld hat sofort Fokus;
mehrere Suchwörter filtern Namen/Kategorie/Beschreibung. Pfeile wählen,
Enter platziert über vorhandenes Undo, Esc bricht ab. Letzte und häufige Bauteile
werden lokal bevorzugt. Bibliotheksbauteile verwenden dieselbe Palette;
während der Simulation bleibt die Schaltung gegen Platzierung gesperrt.
Shortcut ist in Einstellungen anpassbar. / The finder uses the existing palette
and undo architecture, including local component packages, without editing a running circuit.

## Projekt teilen / Sharing

Datei → Projekt teilen: optional Passwort, Code erzeugen, explizit kopieren oder
QR-PNG speichern. Datei → Aus Share-Code importieren: einfügen, Bild öffnen oder
Kamera wählen, Passwort, Vorschau prüfen, **als neues Projekt bestätigen**.
Vorschau zeigt Boards, Namen, Bauteile, Format und Bibliotheksverweise.
Bibliotheken werden weder eingebettet noch automatisch installiert. Keine
Netzübertragung, kein Konto, kein Server. Kamera bleibt lokal und braucht ggf.
Betriebssystem-Berechtigung. Für große Projekte den Code statt QR verwenden.

**EN:** Share codes are generated locally. Preview and explicit confirmation precede
import into a new unsaved project. Existing files are not overwritten. Libraries
are references only. Without a password the code is not confidential. Encryption
protects contents, not sender identity or execution of imported native sketches.
See [format and security](SHARE_CODE.md). Camera hardware needs separate physical
verification beyond automated headless tests.

## Quack / Nyra / Nyra Quack

Einstellungen → Allgemein: Quack-Assistent Aus/Nur Fehler/Normal/Ausführlich,
Skin Quack, **Nyra** oder **Nyra Quack**. Kompakte Hinweise im Diagnose-Dock, kein Popup über
Code oder Schaltung. Kurze modellbezogene Orientierung, bestätigte Fehler mit
praktischem nächsten Schritt, ausblendbar. / Same professional hints with either
skin; local deterministic rules, no AI service, chat account or network dependency.
