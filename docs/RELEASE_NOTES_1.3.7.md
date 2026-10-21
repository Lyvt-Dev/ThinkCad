# ThinkCad 1.3.7 — Funk, NFC und Akkus / Radio, NFC and batteries

## Deutsch

- Enthält alle Neuerungen und Reparaturen aus 1.3.5/1.3.6, einschließlich Aktoren,
  TFTs, Sensoren, lokalem Teilen und Windows-Updater-Reparatur.
- 17 zusätzliche Katalogeinträge: CC1101, SX1276/RFM95, MCP2515, TJA1050,
  SIM800L, SIM7600-UART-Trägerplatine, NEO-6M, Fingerabdruck-Testmodul,
  Li-ion-18650-/LiPo-1S-/NiMH-Zellen, USB-C-Ladeeingang, TP4056 sowie PN532
  als auffindbarer NFC-Transmitter/Reader-Writer. nRF52840 und zwei SDR-Einträge
  sind ausdrücklich Planungskomponenten.
- Paketbasierte CC1101-/LoRa-Modelle mit SPI, Konfigurationsabgleich und begrenzten
  FIFOs. MCP2515 unterstützt CAN-Loopback; TJA1050 modelliert elektrische Buspegel.
- Offline-UART-Testmodelle für Modem, GNSS und Fingerabdrucksensor. Akkus besitzen
  einstellbaren Ladezustand und Innenwiderstand; TP4056 ist eine DC-Ladenäherung.
- PN532 liest und schreibt synthetische Testtags. Ein zweites Board kann einen
  synthetischen Tag mit UID/NDEF und optionalem Schreibschutz bereitstellen.
  Alte Tag-Auswahlen werden nach Reset, Stop oder Versorgungsausfall ungültig.
- Drei neue verdrahtete Beispiele: LoRa-Paketpaar, NFC-Schreiber und NFC-Tag-
  Emulation zwischen zwei ESP32. Neue flache Illustrationen und Modellhinweise
  in den Eigenschaften.
- Gentoo-Normalinstallation erzwingt keine Entwicklertests mehr. Die Tests bleiben
  separat aktivierbar. Größere Desktop-/Peripherie-Tests erhalten mehr Gesamtzeit;
  funktionale Assertions und einzelne Testfall-Fristen bleiben erhalten.

Die neuen Modelle sind keine vollständige Chip-, Firmware- oder RF-Emulation.
Modems/GNSS/Biometrie verwenden synthetische Testdaten. nRF52840 und SDR besitzen
noch kein Firmware-/IQ-/BLE-Backend. NFC-Tag-Emulation nutzt dokumentierte
`thinkcad*`-Simulatorerweiterungen, keine upstream Adafruit-APIs.
Externe CAN-Arbitration, exakte RF-Ausbreitung und dynamische Akku-Ladezustände
sind nicht implementiert. Details: [Hardware-Module](https://github.com/Lyvt-Dev/ThinkCad/blob/v1.3.7/docs/HARDWARE_MODULES.md).

## English

- Preserves the 1.3.5/1.3.6 features and installer fixes.
- Adds 17 catalog entries for radio, CAN, offline UART test devices, rechargeable
  cells and USB-C/TP4056 charging, plus a searchable PN532 reader/writer.
- CC1101 and SX1276 exchange simulated packets; MCP2515 supports CAN loopback.
  Modem/GNSS/fingerprint modules use synthetic data. Battery SOC is configurable;
  charging is a DC approximation rather than a dynamic battery model.
- PN532 supports synthetic-tag writes and two-board tag emulation, including
  configurable UID/NDEF, read-only pages and selection invalidation on reset or
  power loss. The `thinkcad*` methods are simulator extensions, not upstream
  Adafruit APIs. New wired LoRa and NFC examples demonstrate these behaviors.
- nRF52840 and SDR entries are **planning-only**, without firmware, BLE or IQ
  emulation. Model limits are visible in properties and documented above.
- Gentoo installation no longer forces developer tests; tests remain opt-in.
  Extended aggregate test budgets retain per-case checks and deadlines.
