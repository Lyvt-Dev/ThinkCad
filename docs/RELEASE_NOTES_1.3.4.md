# ThinkCad 1.3.4 – Direkte Bedienung und Stabilität

## Fehlerbehebungen

- ATtiny85: Das Diagnosefenster fragt nur tatsächlich vorhandene Versorgungspins ab. Das Verbinden einer Batterie löst dadurch keine ungefangene `map::at`-Ausnahme mehr aus.
- Verspätete Eingaben ausgeblendeter oder gelöschter Bauteile werden verworfen. Ein noch offenes Modul- oder Analogwertfeld kann nach dem Löschen nicht mehr auf ein unbekanntes Bauteil zugreifen.
- Servos besitzen einen tatsächlichen Wellenwinkel mit begrenzter Geschwindigkeit und Beschleunigung. `Servo.write()` setzt das Ziel; der Arm fährt weich dorthin, auch während `delay()`. Ohne Versorgung oder gültige Impulse wird der Antrieb angehalten.
- Der asymmetrische Servoarm macht 0° und 180° sichtbar unterscheidbar. Die realistische Zeichenfläche umfasst seinen gesamten Schwenkbereich.

## Direkte Bedienelemente

- Joystick auf der Arbeitsfläche ziehen; beim Loslassen zentriert er sich. Die integrierte Taste wird über Rechtsklick oder die SW-Fläche gedrückt und beim Loslassen freigegeben.
- Schiebe- und Drehpotentiometer, Drehgeber, Drehschalter, DIP-Schalter, Patchmatrix, Tastenfelder und Touch-Flächen lassen sich direkt bedienen.
- Arcade-Taster, Drum-Pads und Drucksensoren reagieren auf Drücken und Loslassen. Das Freigeben beim Fokusverlust verhindert hängende Eingaben.
- Sensoren und Signalquellen erhalten frei verschiebbare Live-Schieberegler statt des Hinweises, den Wert anderweitig zu verändern.

## Versorgungsanschlüsse

VIN für Uno/Nano und NodeMCU, 5V für ESP32-Varianten und STM32 Blue Pill,
sowie VBUS/VSYS für Pico. Neue Anschlüsse sind verdrahtbar, sichtbar beschriftet
und haben passende Spannungsgrenzen. Uno/Nano-VIN bleibt ein separater Eingang;
Pico-VSYS erhält USB-Strom über eine Schottky-Diode. Alte Kontaktkoordinaten
bleiben stabil. [Modellverhalten und Boardübersicht](BOARD_POWER.de.md).

## Bauteilzeichnungen

Erkennbare flache Darstellungen für Joystick mit Gimbal und beweglichem Knopf,
Sensoren, Ultraschallwandler, Steckverbinder, Audiobuchsen, Touch-/Druckflächen,
Verstärker-, DAC-, Codec- und DDS-Module. Anschlüsse bleiben mit ihren
Simulationspins verknüpft. Korrigierte Steckverbinder- und Sensorabmessungen
werden beim Laden bestehender Projekte berücksichtigt: auseinandergerückte
Direktkontakte bleiben als sichtbare Kabel elektrisch verbunden.

Die dokumentierten elektrischen und Firmware-Modellgrenzen gelten weiterhin;
diese Fehlerkorrektur fügt keine vollständige Transistorebene oder proprietäre
DSP-Firmwareemulation hinzu.
