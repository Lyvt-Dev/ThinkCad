# ThinkCad 1.0.0

ThinkCad erweitert den Schaltungseditor um analoge Simulation, Messinstrumente,
LCDs, Logikgatter, Aktoren sowie IR-, NFC- und Funkversuche. Windows- und Linux-
Änderungen gehören zu demselben Release; das Gentoo-Overlay enthält ein passendes
1.0.0-Ebuild mit Qt SerialPort als zusätzlicher Abhängigkeit.

## Schaltungen und Messungen

- Zustandsbehaftete Kondensatoren/Spulen, generische BJT-/MOSFET-/Op-Amp-Modelle,
  PWM, Masse, Funktionsgenerator und elektrische Überlastdiagnosen.
- Oszilloskop, 8-Kanal-Logikanalysator und Stromsonde: Trigger, Cursor, Messwerte,
  A−B, CSV/PNG-Export sowie UART-/SPI-/I²C-Dekodierung erfasster Signale.
- LiquidCrystal mit 16×2/20×4, 4-Bit-Verdrahtung oder I²C-Backpack; Text und
  eigene Zeichen. Versorgte AND/NAND/OR/NOR/XOR/XNOR/NOT-Gatter.
- Servo.h, Servowinkel aus PWM-Pulsen und DC-Motor mit Gegen-EMK/Trägheit.
- IR-LED, 38-kHz-Empfänger und anklickbare NEC-TV-Testfernbedienung.
- PN532-I²C-Leser mit kreditkartenförmigem synthetischen UID-/NDEF-Test-Tag.
- nRF24L01+ mit RF24-API, Kanälen, Adressen, Datenraten, ACK und Empfangs-FIFO;
  zusätzlich separate 433-MHz-ASK-Sender/-Empfänger mit rc-switch-Protokoll 1.
- Verdrahtete Beispielschaltungen und 58 Bauteiltypen einschließlich Board-Typ.

## Editor und Projektarbeit

- Kopieren/Ausschneiden/Einfügen/Duplizieren einschließlich interner Kabel und
  Board-Sketches; Ausrichten/Verteilen, Raster, Netzlabel und Kabelabzweigungen.
- Autosave je Sitzung, Wiederherstellung und portable Projekte mit geprüften
  Bibliotheksdateien. Projektformat 3 liest ältere Formate 1 und 2.
- Pause/Zeitschritt, serieller Monitor/Plotter für Simulation oder echte Ports.
- Komplette Sketch-Projekte mit Arduino CLI kompilieren und nach erfolgreichem
  Build über einen gewählten COM-/tty-Port hochladen; Abbruch und Buildprotokoll.
- Stücklisten als CSV, Offline-Handbuch auf Deutsch/Englisch und Screenshot-Galerie.

## Modellgrenzen und Prüfung

Die Simulation verwendet native Sketch-Prozesse und ausgewählte Arduino-API-
Adapter. Sie ist keine vollständige CPU-, SPICE-, HF-, EMV-/Bankkarten- oder
Peripherieemulation. NFC-Testdaten sind synthetisch. IR/ASK übertragen modellierte
Nachrichten; nRF24 modelliert Pakete, keine Funkreichweite oder Störungen.
Debugger/CPU-Stepping, PCB-Layout/KiCad, vollständige Interrupts, zusätzliche
Sensoren und beliebige Hardwarebibliotheken sind damit nicht implementiert.

Vor Veröffentlichung wurden die 16 Linux-Testsuiten, reale AVR-Kompilierung mit
Originalbibliotheken und ein installierter GUI-Smoke-Test geprüft. Windows-,
Ubuntu-/Fedora- und Gentoo-Pakete werden durch die Release-Workflows gebaut und
geprüft. Physische ESP32-, Funk- und USB-Tests sind weiterhin nötig.

[Funktionsrundgang und Screenshots](V1_REVIEW.md) ·
[Handbuch](V1_GUIDE.de.md) · [Modulanschlüsse und APIs](WIRELESS_MODULES.de.md)
