# Visual Coding, Breadboard Intelligence & Measurement

ThinkCad 1.3.1 remains a local native C++20 / Qt application. Opening a project
never runs a sketch. Choose **Start simulation** to compile/run it.

## Start with a working block program

Open **Examples → Starter: blinking LED** or **Starter: button and LED**.
Both have complete wiring and an editable block program. Select a board above
its code tabs. Each board retains its own sketch, language and block metadata.

The coding selector offers **Blocks**, **Blocks + C++** and **C++**. Blocks target
Arduino C++; MicroPython remains available through the existing language selector.
The block editor is disabled for MicroPython and while simulation is running.

The graphical workspace shows setup, loop, functions and nested statements.
Select a card to edit its properties. Move cards by dragging or using arrow keys.
Card position affects presentation only. Use the **Structure** tab to select a
statement container and drag statements into execution order or into a branch.
Both views address the same program; the structure tree supports keyboard selection.

Choose **Add block**, select its kind and enter arguments, one expression per
line. **Apply block** validates the program and generates C++. Global variables
have a table editor; functions have named parameters and return types. Types are
signed `int32_t`, `double`, `bool` and `String`. Identifier names use ASCII letters,
digits and underscores. Generated names and operations are validated.

Expressions support literals, variables, function calls, arithmetic, comparisons,
logical operations and parentheses, for example `digitalRead(12) == LOW`.
Strings use double quotes. Raw statements, preprocessor directives and arbitrary
C++ fragments are not accepted as expressions. For loops use an index name and
three expressions: initial value, exclusive limit and nonzero constant step.

Supported commands include digital I/O, ADC, PWM, delay/millis, Serial, one Servo
object, Wire I2C and SSD1306 text. Calls require the correct arguments and valid
board pins. Add initialization calls explicitly in setup. More advanced hardware,
multiple Servo objects and other libraries remain accessible through normal C++.

## Manual C++ and synchronization

Generated code is a normal sketch compiled by ThinkCad's existing compiler
process. The block representation is not a C++ parser. Editing C++ marks the
views as **not synchronized**; it does not update or erase blocks.

Changing blocks does not silently replace manually edited C++. Regeneration asks
before replacing a manual sketch and retains a backup in project metadata.
**Restore manual backup** restores it. This is one recoverable backup, not version
history. Canceling regeneration preserves the sketch. Unknown block schemas are
retained with their C++ source and can only be discarded through an explicit reset.

## Breadboard and diagnostics

Hover over a contact to highlight every electrically joined contact, pin and
wire. While wiring, the preview displays the union of both nets. Breadboard
strips, power rails, switches, pin aliases and named nets share the solver's
connectivity model. Moving components does not change internal contact groups.

Wire crossings and overlapping segments do not create electrical connections.
Create an explicit junction for a branch. Exact duplicate wires are rejected;
collinear overlaps and supply conflicts are warned about. Warning acceptance does
not make an electrically unsafe circuit safe. Existing wire bends, undo/redo and
project persistence remain in use.

Open **View → Diagnostics**. Messages include severity, explanation, solution and
an actionable target; activate a row with Enter or a double click. Rules cover
supply/driver shorts, LED current/return paths, wired floating inputs, device
supplies, I2C conflicts/access failures and known UART TX/TX or RX/RX connections.
Unused unconnected board inputs are omitted to avoid noise. A floating battery
circuit does not require a connection to board GND. Reachability checks are
conservative; the numerical solver remains authoritative for current/voltage.

Runtime UART diagnosis needs actual UART configuration. I2C access diagnosis uses
captured transactions while the panel is visible; an address scan's unacknowledged
empty probes are informational. Unknown electrical detail is not inferred from
sketch text. Diagnosis refresh is bounded and does not run for a hidden panel.

## Measurement: distinguish electrical and protocol data

Add an oscilloscope and connect A/B plus GND; voltage is measured relative to its
GND. The logic analyzer records D0–D7 relative to its GND and configured threshold.
Select timebase, voltage scale, trigger channel/edge and acquisition mode. Freeze
holds the displayed capture while bounded acquisition continues. Click the plot
for alternating cursors; readings include time difference and channel-A difference.
CSV exports the displayed capture, including a frozen capture.

- GPIO, PWM, analog sources and modeled component responses come from the existing
  deterministic transient solver, using virtual microseconds and modeled edges.
- Sampling retains up to 20,000 points per instrument; overwritten samples are
  reported. Missing voltage/reference remains missing, not zero. CSV leaves it blank.
- Bit decoders use captured D0/D1 signals: UART D0, 8N1 at the selected baud;
  SPI D0 clock / D1 data, rising edge, MSB first, without chip-select qualification;
  I2C D0 SCL / D1 SDA. These are bounded educational decoders, not universal analyzers.
- **Protocol view (model)** uses actual modeled UART/I2C/SPI transactions. It does
  not synthesize electrical clock/data edges. Peripheral API transactions do not
  imply CPU-accurate bus waveforms. For those APIs, use the protocol view and the
  existing Simulation Inspector for bytes/registers.
- Resolution is limited by microsecond time, the configured analog step and the
  underlying component model. There is no invented sub-microsecond precision,
  analog bandwidth, probe capacitance or hardware noise model.

## Starter projects

Twelve projects appear in Examples and the learning path: LED blink, button LED,
PWM dimming, servo, potentiometer, OLED temperature, DHT22, HC-SR04, WS2812B,
L298N motor, I2C scanner and two-board UART. Each has wiring, source, learning goal,
expected behavior and an exercise. The original eight gated lessons remain intact;
the starter projects are freely accessible practice projects.

## Local export and compatibility

**File → Export** provides PNG/SVG of the current view, CSV bill of materials and
a project ZIP. The archive contains `project.thinkcad`, per-board sketch folders,
`bom.csv`, `libraries.json` and a README. Library names/versions refer to the local
compiler library roots, not absolute paths. Library files are optional. The SDK
comes with ThinkCad. ZIP uses standard stored entries with CRC32, UTF-8 names and
bounded sizes; exporting does not upload anything.

Project format 8 preserves formats 1–7 on input. It adds optional per-board visual
metadata, including graph positions, generator version, fingerprint and backup.
Older ThinkCad releases cannot read format 8. Transient topology caches and running
measurement histories are not embedded in the project. LAN sharing, recovery and
board copy/paste preserve visual metadata; received code is never auto-executed.
