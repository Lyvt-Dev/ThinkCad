# ThinkCad 1.3.8

## Deutsch

IR-Sendungen laufen jetzt über zeitlich definierte Trägerbursts und Pausen.
Empfang und Decodierung hängen von der Pulsfolge, Trägerfrequenz, elektrischen
Versorgung, Signalstärke, Reichweite und expliziter Sichtblockade ab. Der alte
direkte NEC-Nachrichtenweg wurde ersetzt.

- NEC inklusive Extended-Adressen, invertierten Bytes und kurzen Repeatframes;
  Samsung32, Sony SIRC 12/15/20, RC5, RC6 Mode 0 und Kaseikyo/Panasonic.
- Mehrere strombegrenzte IR-LEDs pro GPIO; unabhängige Sender bleiben getrennt.
  Optische Beiträge eines Ausgangs addieren sich; überlappende Sendungen werden
  als Kollision verworfen. Dauerspannung erzeugt kein IR-Kommando.
- IR-Debugger unter Ansicht: Träger/Hüllkurve, RX-Ausgabe, Timing, Signalstärke,
  Decoderstatus, Mikrosekundenzoom und CSV-Export.
- ESP32-LEDC-Adapter für bis zu 100 kHz, virtuelle Hardwaretimer und Pull-downs.
- Arduino-Binärkonstanten, kompatible IRremote-Aufrufe und Raw-Timing-Captures.
- LCD-Schreib-/Clear-/Home-Zeiten und Unterdrückung unveränderter Frames.
- Optionales deterministisches Tasterprellen; normale Taster bleiben momentane
  Kontakte mit unabhängigem Press-and-hold-Verhalten.
- Verdrahtetes ESP32-C3-IR-Labor mit vier LEDs, vier Tastern, Empfänger und 16×2-LCD.

**Modellgrenzen:** Das ist keine vollständige ESP32-CPU-, RMT- oder analoge
AGC-Emulation. Beliebiges selbst bitgebangenes GPIO/LEDC wird noch nicht in
optische Empfangsframes rekonstruiert. Optische Reichweiten sind Canvas-Einheiten;
Reflexionen und Bauteilblickwinkel sind nicht kalibriert. Die dokumentierten
Protokoll-/API-Varianten sind unterstützt; andere Varianten werden nicht still
als erfolgreiche Frames ausgegeben. Details und betroffene Quelldateien:
[IR-Dokumentation](https://github.com/Lyvt-Dev/ThinkCad/blob/v1.3.8/docs/INFRARED.md).

![IR laboratory and waveform debugger](https://raw.githubusercontent.com/Lyvt-Dev/ThinkCad/v1.3.8/docs/ir-debugger-preview.png)

[38 kHz carrier screenshot](https://raw.githubusercontent.com/Lyvt-Dev/ThinkCad/v1.3.8/docs/ir-carrier-preview.png)

Bestehende Projekte bleiben lesbar. Neue Eigenschaften sind optional.
Windows-Installer, portable AppImage, DEB/RPM, Quellpaket und Gentoo-Overlay
werden gemeinsam aus demselben Commit veröffentlicht.

## English

Infrared now uses virtual-microsecond carrier bursts and spaces, with electrical
checks, optical attenuation and pulse-derived decoding instead of direct NEC
messages. Includes NEC/extended NEC/repeats, Samsung32, Sony SIRC, RC5, RC6 mode 0,
Kaseikyo/Panasonic, a waveform debugger with CSV export, independent IR emitters,
ESP32 LEDC/timer adapters, Arduino binary constants, LCD command timing and optional
button bounce. A wired ESP32-C3 example combines four emitters, four buttons,
a receiver and a 16×2 I²C LCD.

This is a bounded carrier-envelope model, not complete CPU/RMT/AGC emulation.
Arbitrary GPIO bit-banging is not yet reconstructed into optical receive frames.
See the linked model documentation for supported API variants and remaining limits.
