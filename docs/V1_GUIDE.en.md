# ThinkCad – new workflows

This manual describes ThinkCad 1.2.0 and its model limitations.

## Circuit editing

Search for components, drag them from the palette onto the canvas, then connect their pins. Clicking a palette entry does not place it. Select the Properties tab explicitly to edit a selection. Ctrl+C/X/V copies, cuts and
pastes; Ctrl+D duplicates. Internal wires, bends, properties and board sketches
travel with copied components. External connections are omitted. Matching net
labels remain electrically connected after copying. Edit offers alignment and
distribution. R rotates; Delete removes; Ctrl+Z/Y undoes/redoes edits.

Right-click a wire → Create wire junction splits it at an explicit connection
point and starts a new branch. Crossed wires remain separate without a junction.
Right-click a pin → Set net label joins matching, case-sensitive names; letters,
numbers, dots, underscores and hyphens, up to 48 characters. Highlight net selects
its wires. Rename a selected net label in Properties and press Enter to apply.
Settings controls grid spacing/snapping; hold Alt to bypass snapping
while moving. Component properties have their own dock, including resistor ratings.

## Simulation and checks

Ctrl+R/F5 starts/stops; F6 pauses. Time step integrates 100 µs for analog circuits.
Sketch stepping proceeds to the next delay/yield segment that advances virtual
time. This is cooperative stepping, not CPU instruction debugging. A pending
short sketch time segment may finish after Pause. Settings → Simulate for duration
limits a run; sketches may overshoot by their last delay segment.

View → Circuit checks lists errors and warnings. Activate a component-related
entry to select it. Checks include source conflicts, GPIO overload, overvoltage,
op-amp wiring and resistor dissipation. Rating warnings do not stop simulation;
heating and damage are not simulated. A clean report is not complete hardware
validation.

## Instruments

Open the wired RC, RL, op-amp, NPN and PWM examples. Function generator properties
include sine/square/triangle/sawtooth, peak amplitude, offset and duty cycle.
The instrument dock appears when an instrument is added. Scope A/B are relative
to GND; current probes connect in series; logic D0…D7 share GND and have an
adjustable voltage threshold. Z means missing data, not zero.

Measurements cover the acquired samples: min/max, peak-to-peak, RMS, and frequency/
duty when enough edges exist. Select free run, Auto, Normal or Single triggering,
A rising/falling edge and level; Arm restarts acquisition. Normal waits for a
trigger, Auto falls back to rolling data, Single freezes a capture. A−B adds a
math trace. Click twice in the plot for time cursors, Δt, 1/Δt and ΔA. Freeze,
CSV and PNG retain/export the capture/view. Each instrument retains 20,000 samples.

Logic decoding: UART D0 (8N1, configurable baud), SPI D0=clock/D1=data (rising
edge, MSB first, bytes without chip select), I²C D0=SCL/D1=SDA (start/stop,
address/data bytes and ACK/NACK). These decode captured logic levels; they do not
implement full MCU peripheral emulation. Adequate samples per bit are essential.

Analog models use Backward Euler, Ebers-Moll BJTs, level-1 MOSFETs and a simplified
rail-limited op-amp. Configure the maximum analog step under Settings and compare
results with a smaller step. Manufacturer SPICE models, AC sweeps, temperature
and CPU timing are not implemented. See ANALOG_SIMULATION.md for model details.

## LCDs, logic and actuators

16×2 and 20×4 LCDs support LiquidCrystal with 4-bit wiring and LiquidCrystal_I2C
backpacks. Use the wired examples. Parallel: 5 V VCC; GND/RW to ground; VO to
ground or a low contrast voltage; RS/E/D4…D7 to the constructor pins. I²C requires
matching supply, SDA/SCL and address (default 0x27); backpack pullups connect to VCC.

Supported: Print/text, clear/home/setCursor, display/noDisplay, cursor/blink,
scrolling, direction, autoscroll and eight custom 5×8 glyphs. I²C adds init/begin
and backlight. Wiring is checked; these are rendered API adapters, not cycle-
accurate HD44780 signaling or PCF8574 register emulation. Unsupported raw LCD
commands report an error.

AND/NAND/OR/NOR/XOR/XNOR/NOT are generic powered gates (1.8–5.5 V). LOW ≤0.3×VCC;
HIGH ≥0.7×VCC. Uncertain inputs leave the output undriven. No propagation delays,
fan-out limits or manufacturer-specific 74HC packages are modeled.

Servo.h supports attach/write/writeMicroseconds/read/detach. Supply 4.5–6 V;
SIGNAL receives 50 Hz pulses. Displayed angle follows measured pulse widths;
servo gearing, load and inertia are not modeled. DC motors model armature
resistance, inductance (default 1 mH), back-EMF and mechanical acceleration.
Use a driver for a motor, not a GPIO. Integrated H-bridges, encoders and mechanical
assemblies are outside this build.

## Serial monitor and plotter

View → Serial tools selects a simulated board or physical device. Choose a port
and baud rate before connecting hardware. Simulated Serial supports Print/
println/write and available/read/peek. Send text with LF, CRLF, CR or no ending.
The plotter accepts numeric lists or name:value, e.g. Voltage:3.3 Current:0.02.
Up to 16 values per line and 2,000 points; the monitor retains 10,000 lines.
Plot channel names include their source.

## Compile and upload to hardware

File → Compile / upload hardware uses an installed Arduino CLI, board core and
libraries. Editable FQBN defaults: Uno arduino:avr:uno; classic Nano
arduino:avr:nano:cpu=atmega328old; ESP8266 esp8266:esp8266:nodemcuv2;
ESP32 esp32:esp32:esp32; S3 esp32:esp32:esp32s3. Select the correct Nano bootloader
or other board variant when necessary.

All .ino/.h/.cpp files build in a temporary sketch using real board libraries,
not the simulation SDK. After a successful compile, select the correct port and
upload. Simulation and its serial connection are stopped first. Logs and Cancel
are available. Upload requires a physical board. Linux needs access to its tty;
Windows may need the board's USB driver. Device permissions are not changed for you.

## Projects, recovery and sharing

Unsaved changes autosave every 15 seconds to a separate session file, leaving
saved projects untouched. File → Recover projects lists snapshots from closed
sessions. Recover opens an unsaved copy; save it under a new name. Session locks
exclude active windows. Normal saving or deliberate discarding removes that
session's snapshot. An unwritable application data directory falls back to a
temporary directory; system cleanup can remove those files. Autosave is not backup.

Export portable project bundle embeds configured/project libraries, their
library.properties metadata and SHA-256 checksums. Recipients do not need those
library folders. Board cores, Arduino CLI and system tools remain external.
Limits: 20 MiB/2,000 library files, 32 MiB total project. Respect library licenses
when sharing. Execute only trusted projects and libraries.

Project format 3 reads formats 1 and 2. Project information shows library metadata
and missing configured directories. BOM export writes components, values, models
and quantities to CSV; it is not a PCB/footprint export. Full PCB tools and CPU/
firmware emulation remain later development work.

## IR, NFC and radio

File → Example circuits includes a NEC TV remote, IR LED loopback, PN532/test
card, two ESP32s with nRF24L01 radios, and a 433 MHz ASK pair. Examples are fully
wired. View → Serial tools shows their results. Modules check supply and bus
wiring. The chosen default is ESP32 + PN532 + nRF24L01+, a 2.4 GHz radio;
CC1101/sub-GHz transceivers and 125 kHz RFID are not implemented.

Use IRremote.hpp's modern IrSender/IrReceiver NEC API, Adafruit_PN532's I²C API,
RF24, or RCSwitch protocol 1. The TV remote sends on a rising button press;
click during simulation or use its inspector checkbox, then release before pressing again.
Command defaults to 0x45 and address to 0. IR sender LED needs a series resistor.
The NFC card has a synthetic 4/7-byte UID and read-only Type-2 NDEF text, not
payment credentials or EMV. Click during simulation removes/presents it; select it to edit
UID and NDEF, then Apply. Only one nearby card in the same test group is selected.

ESP32 wiring: IR TX2/RX4; PN532 SDA21/SCL22/IRQ27/RST26; nRF24 SCK18/MISO19/
MOSI23/CSN5/CE16; ASK TX25/RX26. All example supplies use 3V3/GND. nRF24 must
never use 5V. The ASK RX pin overlaps NFC RST in these separate examples; move
it for a combined circuit. Check actual breakout voltage/pin requirements.

nRF24 supports channels 0–125, 250/1000/2000 kbit/s, five-byte addresses,
1–32-byte payloads, RX pipes, matching CRC configuration, auto-ACK and a three-
packet receive FIFO. Firmware configuration overrides its initial inspector
channel. There is no RF propagation, interference, collision, raw SPI-register
or CPU emulation. PA settings validate but do not change range; retry settings
model failure waiting time, not repeated transmissions. IR/ASK adapters exchange
messages rather than raw pulse/carrier waveforms. IR and ASK have eight-message
queues. IR range is 1,400 scene units; NFC 320; ASK 3,000; test groups must match.
These distances are test controls, not measured physical ranges. Unsupported
library APIs remain unavailable; installing a library does not add device physics.

For hardware builds install original IRremote, Adafruit PN532 (and Adafruit
BusIO), RF24 and rc-switch libraries plus the real ESP32 core. Firmware upload
uses original libraries, not these SDK adapters. Detailed API and wiring tables
are in the bundled [module reference](WIRELESS_MODULES.de.md).

## Workspace improvements in 1.2

Right-click empty canvas space to add a note; double-click a note to edit it.
Notes are saved with the project. Once saved to a named file, changed projects
automatically save to that file every minute (configurable in Preferences).
The separate recovery snapshots remain available. During simulation, click
interactive controls directly; wires and static parts cannot be selected.
Simulation speed keeps its slider and also accepts a typed multiplier from
0.1× to 10×. Open Tutorials from the start menu for eight guided exercises.
Settings → Preferences groups appearance, saving and simulation options.

For portable Linux installation and updates, see [AppImage instructions](APPIMAGE.de.md).
