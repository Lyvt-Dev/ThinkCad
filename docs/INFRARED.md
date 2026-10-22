# IR- und ESP32-C3-Simulation (1.3.8)

## Architektur und Umsetzung

ThinkCad kompiliert Arduino-C++ gegen sein Host-SDK in einen eigenen Prozess.
`Arduino.h` führt eine virtuelle Mikrosekundenuhr; der Desktop empfängt GPIO-,
Zeit- und Peripherieereignisse. Die Schaltung wird über verbundene elektrische
Netze und einen analogen Solver ausgewertet. Das ist keine RISC-V-CPU-Emulation.

Vor 1.3.8 schickte `sendNEC()` Adresse und Kommando direkt an eine Empfängerwarteschlange.
Es gab keine Pulsdecodierung, und genau eine IR-LED musste zum Ausgang passen.
Die neue Implementierung arbeitet in vier überprüfbaren Schritten:

1. `sdk/irwaveform.hpp`: Protokollkodierung in Trägerbursts und Pausen, unabhängige
   Pulsdecoder, analytische Trägerflanken und optische Filter.
2. `sdk/IRremote.hpp` und `src/gui/peripheralhub.cpp`: begrenzter Wellenformtransport,
   elektrische LED-Prüfung, optische Beiträge und zeitverzögerte Empfangscaptures.
3. `src/gui/irvisualizer.hpp`: tatsächliche Ereignisdaten mit Zoom und CSV-Export.
4. ESP32-Timer/LEDC, Arduino-Konstanten, LCD-Zeiten, Tasterprellen und Integrationstests.

## Protokolle und APIs

Unterstützt: NEC (8-/16-Bit-Adresse, 8-Bit-Kommando, invertierte Bytes und kurze
Repeatframes), Samsung32 (8-/16-Bit-Adressen und Kommandos), Sony SIRC
(12/15/20 Bit), RC5, RC6 Mode 0 und Kaseikyo/Panasonic (48 Bit, Herstellerparität
und Prüfsumme). Andere RC6-Modi, Samsung48, Pronto und alte `decode_results`-
APIs sind nicht implementiert. Unbekannte oder beschädigte Frames werden nicht
als erfolgreiche Kommandos ausgegeben.

`IrSender.begin(pin)`, `sendNEC`, `sendSamsung`, `sendSony`, `sendRC5`, `sendRC6`,
`sendKaseikyo`, `sendPanasonic`, `sendRaw`, `enableIROut(kHz)`, sowie
`IrReceiver.begin`, `decode`, `resume`, `start`, `stop` und `decodedIRData` sind
verfügbar. `rawDataPtr->rawbuf` verwendet 50-µs-Ticks; die interne Capture bleibt
mikrosekundengenau. Der anfängliche Abstandswert ist derzeit 0 (nicht gemessen).
`setDutyCycle(percent)` ist eine ThinkCad-Erweiterung (10–60 %).

Die Standardträger sind NEC/Samsung 38 kHz, Sony 40 kHz, RC5/RC6 36 kHz und
Kaseikyo 37 kHz. `enableIROut()` überschreibt sie; 56 kHz ist ebenfalls möglich,
benötigt jedoch einen passend konfigurierten Empfänger. Wiederholungen sind auf
acht begrenzt. NEC-Repeats benötigen ein vollständiges vorheriges Frame innerhalb
150 ms. `resume()` gibt das gehaltene Capture frei.

Referenz: Arduino-IRremote **4.7.1**, insbesondere
[NEC](https://github.com/Arduino-IRremote/Arduino-IRremote/blob/v4.7.1/src/ir_NEC.hpp),
[Sony](https://github.com/Arduino-IRremote/Arduino-IRremote/blob/v4.7.1/src/ir_Sony.hpp),
[RC5/RC6](https://github.com/Arduino-IRremote/Arduino-IRremote/blob/v4.7.1/src/ir_RC5_RC6.hpp)
und [Kaseikyo](https://github.com/Arduino-IRremote/Arduino-IRremote/blob/v4.7.1/src/ir_Kaseikyo.hpp).
NEC-Nennwerte: 9000/4500 µs Leader, 560 µs Mark, 560/1690 µs Bitpause,
9000/2250/560 µs Repeat, 110 ms Startabstand. Der Decoder erlaubt ungefähr
22 % Pulsabweichung (Manchester-Zellen 15 %); Tests verändern Pulse um ±6 %.
Das sind Protokollreferenzen, keine Messungen an einem konkreten TSOP-Modul.

## Verdrahtung und Optik

IR-LED: GPIO → eigener Vorwiderstand (z. B. 220 Ω) → A; K → GND.
Jede LED wird separat auf Vorwärtsspannung und Strom geprüft. Der Bereich
1–50 mA ist zulässig; ohne Strombegrenzung, bei falscher Polung oder unterbrochener
Verdrahtung erfolgt keine Übertragung. Schaltungen über Transistoren können
funktionieren, wenn der Schaltungssolver den durch den GPIO geschalteten LED-Strom
korrekt auflöst. Das ist keine Garantie für beliebige Treiberschaltungen.
Mehrere LEDs an einem Pin müssen eigene Strombegrenzungen besitzen. Ihre optischen
Beiträge werden summiert; getrennte Pins bleiben getrennte Quellen. Ein dauerhaftes
GPIO-HIGH sendet kein Kommando.

Empfänger: bei ESP32 VCC an 3V3, GND an GND, OUT an einen Eingang.
5 V eignen sich für 5-V-Board-Eingänge; ESP32-Eingänge sind nicht 5-V-tolerant. OUT ist aktiv LOW
während erkannter Bursts. Der Standardempfänger liegt bei 38 kHz mit ±12 %
Trägertoleranz. Das Dropdown „IR receiver fidelity“ wählt Standard (mindestens
sechs Trägerzyklen/Burst, Stärke 0,15) oder Strict (zehn Zyklen, Stärke 0,25,
maximal ±10 % Trägerabweichung). Beide verwenden kompakte Hüllkurven und keine
CPU-teure Einzel-Flankensimulation. Empfänger und Sender müssen dieselbe Testgruppe verwenden.

In den Eigenschaften stehen Reichweite, Empfindlichkeit, relative TX-Intensität,
Trägerfrequenz, Toleranz, Umgebungsdämpfung und eine explizite Sichtblockade zur
Verfügung. Die Stärke nimmt mit Entfernung ab; mehrere gleichzeitige LEDs am
selben Ausgang addieren ihre Stärke. Überlappende unabhängige Übertragungen werden
als Kollision verworfen. Die Reichweite verwendet **Canvas-Einheiten**, keine
kalibrierten Meter. Andere Bauteile sind keine automatisch erkannten optischen
Wände. Umgebungs-IR ist eine deterministische Dämpfung, kein Zufallsrauschen.

## Debugger und Zeit

**Ansicht → IR signal debugger** öffnet die Capture-Historie. Sie zeigt Ausgang,
optische Burstfolge, demoduliertes Empfangssignal, Trägerfrequenz, Tastgrad,
Signalstärke, Decodierstatus, Protokoll, Adresse und Kommando. Zoom und Offset sind
in virtuellen Mikrosekunden. Bei grobem Zoom zeigt die GPIO-Spur die Hüllkurve;
unter 2 µs/Pixel werden Trägerflanken ausgewertet. CSV enthält alle Bursts/Pausen
mit absoluten Startzeiten. Die Grafik erzeugt keine Simulationsereignisse.

Die Mikrosekundenuhr zählt virtuelle Zeit, unabhängig von Bildrate und Tempo.
Pause und Einzelschritt bleiben über den bestehenden Prozess-/SPEED-Handshake
verfügbar. Ein IR-Frame ist ein kompakter Block von höchstens 256 Abschnitten und
250 ms. Es gibt ein gehaltenes Capture je Empfänger (Freigabe mit `resume()`) und 64 historische
Traces; alte Einträge werden verworfen. 38 kHz erzeugt keine 76000 Qt-Signale/s.

LEDC unterstützt häufige Arduino-ESP32-2.x-APIs sowie explizite
`ledcAttachChannel`/`ledcWriteChannel`-APIs. Nach `ledcAttach`/`ledcAttachChannel` oder mit `ESP_ARDUINO_VERSION_MAJOR=3`
verwendet `ledcWrite` Pinsemantik. Sechs Kanäle, 1–16 Bit und 1–100 kHz sind
modelliert; echte Taktteiler-/Auflösungsgrenzen werden nicht emuliert.
Timer unterstützen Mikrosekundenfristen, One-Shot/Auto-Reload und Aufräumen.
Endliche Reload-Zähler werden ausdrücklich abgelehnt. Callback-Zeitpunkte sind
virtuelle Fristen, keine modellierten CPU-Interruptlatenzen.

## LCD, Taster und Arduino-C++

Das Beispiel **ESP32-C3: IR waveform lab** enthält vier IR-LEDs mit vier
Vorwiderständen (GPIO2/4/5/6), RX GPIO7, Taster GPIO0/1/3/10 gegen GND,
LCD 16×2 an SDA8/SCL9, Adresse 0x27. Es sendet beim Start ein Testkommando;
anschließend wählt jeder Taster einen eigenen Sender und ein Kommando aus.

`LiquidCrystal_I2C` unterstützt Cursor, acht CGRAM-Zeichen und Hintergrundlicht.
Zeichenschreibzeiten sind 37 µs, clear/home 1520 µs; unveränderte Frames werden
unterdrückt. Das ist ein Befehlsadapter, keine vollständige PCF8574-Nibble-Busspur.

Taster sind momentane Kontakte mit gemeinsam verbundenen Beinpaaren und
unabhängigen Zuständen; gedrückt halten und mehrere Taster funktionieren.
Optionales Kontaktprellen (0–20000 µs) schaltet den Kontakt deterministisch in
250-µs-Abschnitten. 0 erhält das bisherige ideale Verhalten. Es ist kein gemessenes
mechanisches Zufallsmodell. INPUT_PULLUP und INPUT_PULLDOWN werden elektrisch
über schwache 47-kΩ-Widerstände modelliert.

Die Arduino-Konstanten B0 bis B11111111 einschließlich führender Nullen sind
verfügbar. Vorhandene Sketch-Prototyperzeugung unterstützt gewöhnliche globale
Funktionen und vorab definierte Enum-/Strukturtypen. Komplexe Templates,
Makro-generierte Signaturen und Typen, die erst nach ihrer Benutzung definiert
werden, können explizite Prototypen/Header benötigen; das ist kein vollständiger
arduino-preprocessor-Ersatz.

## Verbleibende Grenzen

* CPU-Instruktionszeiten, RMT-Register, FreeRTOS, DMA, echte Interruptprioritäten
  und beliebige externe Arduino-Binärbibliotheken werden nicht emuliert.
* Die optische Engine verarbeitet die Frames der unterstützten IRremote-APIs;
  beliebiges selbst bitgebangenes GPIO oder LEDC wird noch nicht automatisch
  in IR-Empfangsframes rekonstruiert. Solche Signale können elektrisch betrachtet
  werden, sind aber kein Ersatz für den hier implementierten IR-Sendeadapter.
* Temperatur, Wellenlängenspektren, AGC-Aufwärmzeit, Reflexionen, Blickwinkel und
  Übersteuerung eines konkreten Empfängermodells sind nicht kalibriert.
* Es gibt keine vollständige analoge AGC-Simulation und keine stochastische
  Rauschquelle. Einstellbare optische Parameter ersetzen kein echtes Datenblatt.
* Die GPIO-/RX-Captures im IR-Debugger zeigen die geplanten Wellenformen.
  GPIO-Träger und RX-Hüllkurve werden auch im elektrischen Solver geführt.
  Messinstrumente können diese Flanken erfassen. Die Zustellung von GPIO-
  Interruptcallbacks bleibt durch den bestehenden SDK-Pollrhythmus begrenzt;
  CPU-Latenzen und beliebige ISR-Bitbang-Decoder sind nicht vollständig modelliert.

Bestehende Projekte bleiben lesbar; neue Parameter sind optional und verwenden
bisherige Standardwerte. Tests prüfen Protokolle, Timing, Trägerperioden,
Fehlerframes, Optik, Mehrfach-LEDs, Puffergrenzen, Timer, Pull-Widerstände,
Kontaktprellen und den kompletten C3-LCD-IR-Sketch. Reale Hardwaretests bleiben
für die genannten analogen und MCU-spezifischen Grenzen notwendig.

## Geänderte Dateien

* Protokolle/Arduino: `sdk/irwaveform.hpp`, `IRremote.hpp`, `IRremote.h`,
  `Arduino.h`, `binary.h`, `esp32timing.hpp`, `LiquidCrystal.h`.
* Elektrik/Zeit: `include/thinkercad/simulation.hpp`, `circuit.hpp` sowie
  `src/core/simulation.cpp`, `circuit.cpp`, `transient.cpp`. Dazu gehört die
  Stabilisierung des Dioden-Knickpunkts für ausgeschaltete Transistortreiber.
* Desktop: `src/gui/peripheralhub.cpp/.hpp`, `irvisualizer.hpp`,
  `componentoptions.hpp`, `mainwindow.cpp`, `simulationinspector.cpp`, `examples.cpp`.
* Beispiel/Tests: `examples/esp32_c3_ir_lab.ino`, `tests/ir_waveform_tests.cpp`,
  `esp32_timing_tests.cpp`, `ir_contacts_tests.cpp`, `peripheral_tests.cpp`,
  `desktop_tests.cpp` und die CMake-Testregistrierung.
* Veröffentlichung: README, Versionshinweise, Screenshots, AppStream,
  Windows-Installer-Version und Gentoo-Ebuild 1.3.8.
