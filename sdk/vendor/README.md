# Bundled Arduino source libraries

These unmodified library sources are pinned in `manifest.json`, including a
SHA-256 for every shipped source/header/license. Each directory retains its
upstream license. No downloads are required to use them in ThinkCad.

- Adafruit BME280 2.3.0 — BSD
- Adafruit MPU6050 2.2.9 — BSD
- Adafruit BusIO 1.17.4 — MIT
- Adafruit Unified Sensor 1.1.15 — Apache 2.0
- MFRC522 1.4.12 — Unlicense/public domain
- DallasTemperature 4.0.6 — MIT

The normal dependency scan selects needed source files; unused libraries are not
compiled into a sketch. ThinkCad provides the native Arduino GPIO, Wire, SPI and
OneWire interfaces. Available library API does not imply emulation of unsupported
hardware commands: the device support matrix describes register/protocol limits.

Copyright and license text remain with the respective upstream projects.
