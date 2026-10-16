# ThinkCad 1.2.1

- Running circuits cannot be selected, dragged or rewired. Cable clicks and
  double-clicks are ignored; wiring actions are disabled. Stopping restores
  normal editing. Zoom and panning remain available.
- Pushbuttons and remote buttons respond to one press and release. Switches and
  NFC-card presence toggle with a single click during simulation.
- Added Russian, Ukrainian, Spanish and French alongside German and English.
  UI catalogs and tutorials are bundled offline; language selection persists.
  New installations choose a supported system language, with English fallback.
  The extended reference guides remain available in German and English.
- Added **File / Settings → Convert video to frames.h…**. Exports 128×64 or
  128×32 monochrome Adafruit GFX-compatible PROGMEM frames with adjustable FPS,
  clip range, threshold and inversion. The 256 KiB bitmap limit keeps generated
  headers usable by the ESP8266 and within ThinkCad's sketch-size limit.
- Decoder playback is tested in the application smoke test, including deployed
  Windows builds. A generated header is also compiled and executed by the
  simulation test suite. Codec availability depends on the installed backend.

See [Video export](VIDEO_FRAMES.md) for usage and a complete Arduino example.
