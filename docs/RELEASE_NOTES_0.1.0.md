# ThinkerCad 0.1.0 — First prototype

Native Windows desktop prototype built with C++20 and Qt 6.

## Included

- ESP8266 / NodeMCU and a prewired SSD1306 128×64 OLED.
- Movable 2D components, zoom, sketch editor, serial/compiler output.
- Native C++ execution of `setup()` and `loop()` through a simulator API subset.
- Digital GPIO, ADC inputs, virtual timing, OLED bitmap animations and image import.
- Local portable C++ library support.
- Per-user Windows installer with bundled sketch compiler.

## Installation

Download `ThinkerCad-Setup-0.1.0.exe` and run it on Windows 10/11 x64.
The `.sha256` asset provides its SHA-256 checksum. Matching Qt and compiler
source archives are provided as separate release assets for third-party licensing.

This initial installer may be unsigned. Windows SmartScreen may show an unknown
publisher warning. Builds without a configured publisher certificate have automatic
update installation disabled; they never accept unchecked update installers.

## Prototype limitations

The circuit is prewired; arbitrary circuit wiring is not yet implemented.
This is an Arduino API subset, not an Xtensa firmware emulator. Full ESP8266
libraries, Wi-Fi, PWM, interrupts and general analog circuits are not supported yet.
Run only trusted sketches and libraries: the native worker is not a security sandbox.

See README.md and docs/ARCHITECTURE.md for development and model details.
