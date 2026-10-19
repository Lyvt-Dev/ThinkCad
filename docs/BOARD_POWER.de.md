# Versorgungsanschlüsse der Boardmodelle

Die zusätzlichen Pins sind elektrische Anschlüsse: Kabel und Direktkontakte
können daran angeschlossen werden; die Spannungsprüfung unterscheidet sie von
3,3-V-GPIOs. Bestehende Pin-Namen und Kontaktpositionen bleiben erhalten.

| Board | Zusätzliche Anschlüsse |
| --- | --- |
| Arduino Uno R3, klassischer Nano | VIN; der vorhandene 5V-Anschluss bleibt separat |
| ESP8266 NodeMCU | VIN |
| ESP32 DevKit, ESP32-S3, ESP32-S2, ESP32-C3, ESP32-C6 | 5V |
| STM32F103 Blue Pill | 5V |
| Raspberry Pi Pico / RP2040 | VBUS, VSYS |
| BBC micro:bit, nackter ATtiny85 | Keine erfundenen VIN-/5V-Anschlüsse; vorhandene 3V3-/VCC-Anschlüsse verwenden |

## Bedeutung im Simulator

Entwicklungsboards werden wie bisher als USB-versorgt modelliert. Der 5V- bzw.
VBUS-Anschluss liefert deshalb eine ideale 5-V-Versorgung. Beim NodeMCU wird
auch VIN im USB-Modell mit idealen 5 V versorgt; reale Spannungsabfälle und die
Beschaltung unterscheiden sich zwischen NodeMCU-Varianten.

**VIN am Uno/Nano ist ein Rohspannungseingang, kein 5-V- oder 9-V-Ausgang.**
Er liegt auf einem anderen Netz als 5V und kann beispielsweise mit einer
9-V-Batterie verbunden werden. Das Board bleibt im bestehenden USB-Modell
versorgt; ein Umschalten zwischen USB und externer Versorgung sowie thermisches
Verhalten des Onboard-Reglers werden nicht simuliert.

Beim Pico speist VBUS den VSYS-Anschluss über eine modellierte Schottky-Diode
mit 0,3 V Durchlassspannung. Eine höhere externe VSYS-Versorgung speist nicht in
VBUS zurück. Beide Anschlüsse werden bis 5,5 V geprüft. 5-V-Anschlüsse machen
3,3-V-GPIOs nicht 5-V-tolerant.
