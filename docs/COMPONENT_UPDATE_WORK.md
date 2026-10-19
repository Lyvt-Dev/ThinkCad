# Component expansion — v1.3.3 implementation record

The user authorized publication as v1.3.3. This record preserves implementation scope and verification history.
The existing ESP32-S3 / LiquidCrystal_I2C 3.3 V fix must remain included.

## Requests to implement and verify

- Unified breadboard: 170 / 300 / 630 terminal contacts, optional power rails; one palette entry; preserve legacy documents.
- LED and tactile cap colors; compact flat artwork; pin spacing and proportional physical dimensions.
- CD74HC4067, 74HC165/595, 74HC283, 4017, 93, 4511, 74, 109, 373, 14.
- Optical encoder 600 P/R; 100 mm center-detent and 60 mm B10K sliders; knurled rotary B10K; 12 mm and colored 6 mm buttons; rotary 1P5T; DIP 2/4/6 switches.
- WS2812 60 LEDs/m; NeoPixel single/strip/ring/matrix; discrete configurable LED matrix, 10-segment bargraph, clock display.
- TRRS sockets/cables; many real connector variants; DIN MIDI IN/OUT/THRU; 6.35 mm TS/TRS, RCA, banana; crossbar; SPDT/DPDT toggles.
- micro:bit, breakout, ATtiny85; 3- and 4-pin ultrasonic, PIR, MQ, soil, flex, FSR, ambient light, photodiode.
- Vibration and geared motors, motor + encoder, active/passive piezo.
- AA/AAA packs 1–4 cells; solar, lemon, potato; bulb; polarized capacitor, zener; LM7805, LD1117, SPDT/DPDT relay, 555/556, LM741, LM339/393, 4N35; multimeter V/A/ohms.
- AS3340/CEM3340, XR2206/ICL8038, AD9833/AD9850, white/pink/brown noise.
- AY-3-8910/YM2149F, SN76489, VS1053b MIDI/MP3 codec.
- SSI2144, TL072/TL074, AS13700/LM13700, NE5532/OPA2134, LM386, PAM8403/TPA3116D2, CD4051B/CD4052B.
- PCM5102A, MCP4725, MCP4822/MCP4922, R-2R ladder, WM8731/CS4272.
- FV-1 DSP, PT2399, PT2258.
- MPR121, SoftPot, velocity-sensitive drum pads, analog XY joystick + switch, arcade buttons, 6N138/137.
- TC7660/MAX1044/ICL7660, LM317/LM337, TL431.

## Evidence collected so far

- Original LCD failure reproduced, fixed, tested in both simulation and emulation; three related suites passed before this expansion.
- New core electronics test validates every descriptor, shifts/latches 0xA5 through 74HC595, routes analog voltage through CD74HC4067, and checks breadboard isolation/counts.

This is a working checklist, not a claim that all listed models are complete. Every model needs functional tests and documented bandwidth/device limits before this update is ready.

## Latest verification

The 44-suite regression run passed 43 suites and exposed an obsolete assumption
that every module has a power pin. Passive modules now receive per-pin voltage
checks; the ATtiny85 test explicitly supplies its bare-chip VCC. The corrected
module suite and all 11 affected regression suites passed on the rebuilt code.
Subsequent checks passed the expanded core tests, module/profile validation,
435 audio/PCM checks and all 23 peripheral cases (including the gallery export).
The latest MP3 test feeds two chunks and checks that old samples are not replayed.

Review artifacts are generated locally in `out/component-update/`:
`component-gallery-00.png` through `component-gallery-11.png`, LCD simulation and
emulation screenshots, and `new-breadboard-led-properties.png`.
See `AUDIO_COMPONENTS.de.md` for usage, evidence and explicit outstanding limits.
The component expansion is integrated with the newer editor and updater changes for v1.3.3.
