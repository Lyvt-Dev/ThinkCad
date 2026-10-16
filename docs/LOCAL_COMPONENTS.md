# Local component packages (1.2.6)

ThinkCad component packages are local JSON data. Import a `.thinkcad-components.json`
file or choose a local directory, including a checked-out Git working copy, in
Settings → Local components and plugins. Importing does not contact a repository,
run Git hooks, load native libraries, or execute scripts. The component palette
refreshes after import. Existing builtins cannot be replaced.

A minimal sensor with a read-only identity register and a temperature input:

```json
{
  "format": "ThinkCad.Components",
  "version": 1,
  "models": [{
    "id": "local-temperature-sensor",
    "label": "Local temperature sensor",
    "bus": "i2c",
    "implementation": "registers",
    "pins": ["VCC", "GND", "SDA", "SCL"],
    "pinPositions": {"VCC": [1, 24], "GND": [6, 24], "SDA": [11, 24], "SCL": [16, 24]},
    "widthMm": 18,
    "heightMm": 24,
    "minVoltage": 3.0,
    "maxVoltage": 3.6,
    "address": 72,
    "maxClock": 400000,
    "registers": {"0": 66},
    "readOnly": [0],
    "parameters": [{
      "name": "temperature", "label": "Temperature", "unit": "C",
      "default": 25, "min": -40, "max": 85,
      "register": 16, "bytes": 2, "scale": 100,
      "littleEndian": false, "signed": true
    }]
  }]
}
```

Coordinates and body size are millimetres. If positions are omitted, pins are
arranged along the two sides. Models may use I2C or SPI register banks.
`pullups` optionally maps signal pin names to resistances (330–100000 ohms) to
the supply pin. `idleCurrent` is the approximate idle load at 3.3 V in amperes;
the solver represents this as a resistive load, not a regulated constant current.
Register
addresses and values are bytes. A parameter mapping encodes `value × scale` in
1–4 bytes; signed values use two's complement. Parameter registers represent
inputs and cannot be overwritten by the sketch. Other non-read-only registers
are mutable device state. Bus behavior is documented in the 1.2.6 support matrix.

Packages contain 1–32 models and at most 512 KiB. IDs must be unique; an identical
already-installed descriptor is reused, while a conflicting one is rejected.
Pin, voltage, numeric range and register encoding validation applies to imported
packages and project-embedded descriptors alike. A malformed project rolls back
any descriptors that it temporarily introduced.

Project format 4 embeds definitions for local models actually used by the
circuit, so the recipient does not need the original package folder. Sources,
absolute repository paths and local settings are not embedded. Versions 1–3 of
ThinkCad project files remain readable; they use the previous Simulation mode.

The package manager can save a sample package. Built-in device models remain
implemented and tested in native C++; data packages extend their register-bank
mechanism without requiring changes to the simulation core.
