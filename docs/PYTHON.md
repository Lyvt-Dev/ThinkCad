# Python in ThinkCad

ThinkCad remains a local native C++/Qt application. Python code runs in separate
local processes and starts only when you choose Run or Start simulation.

## MicroPython board sketches

Choose an ESP, RP2040 or STM32 board and select **MicroPython** above its code
tabs. Every board retains its own language and files. Switching to Arduino C++
retains the Python files, and switching back retains the C++ files. A `.py` file
opened through File / Open becomes the MicroPython entry file. Sibling `.py`
files and modules under `lib/` and `src/` are imported locally.

```python
from machine import Pin
from time import sleep_ms

led = Pin(2, Pin.OUT)  # ESP8266 / ESP32 GPIO2; wire an LED with a resistor
while True:
    led.on()
    sleep_ms(500)
    led.off()
    sleep_ms(500)
```

The application bundles a genuine MicroPython 1.26.1 interpreter built from
pinned upstream source. This is a host interpreter connected to ThinkCad's
electrical and peripheral models, not an ESP firmware/CPU emulator. No separate
Python installation is needed for board sketches. Arduino Uno/Nano sketches
continue to use Arduino C++.

Supported adapters:

- `machine.Pin`: input, output, pull-up, value/on/off/toggle and soft GPIO IRQs.
  IRQs are dispatched at GPIO/time boundaries. Pull-downs, alternate functions,
  drive strength and hard IRQs are explicitly unsupported.
- `machine.PWM`: `freq`, `duty`, `duty_u16`, `deinit`; board pin constraints apply.
- `machine.ADC`: `read`, `read_u16`, `read_uv`; resolution and voltage range follow
  the selected board profile. Arbitrary attenuation/calibration is not modeled.
- `machine.I2C` / `SoftI2C`: scan, read/write, read-into and memory-register
  transactions; primary controller, at most 128 bytes per transaction and
  400 kHz. Device power, wiring, address and device-model limits apply.
- `machine.SPI` / `SoftSPI`: byte transfers, read/write/readinto/write_readinto;
  software-managed chip-select pins, bounded transfers and supported devices.
- `machine.UART`: configured physical pins, baud rate/framing, read/write,
  readinto/readline, any and deinit, with bounded buffers and virtual timeouts.
- `time` / `utime`: sleep, sleep_ms, sleep_us, ticks_us/ms, ticks_add/diff. Delays
  honor simulation speed, pause and stepping.

Python drivers can use these APIs with the existing device models. Native
MicroPython extensions, Wi-Fi, Bluetooth, filesystem APIs and every ESP-specific
module are not implied by language support. Unsupported imports fail visibly.
The bridge modules `machine.py`, `time.py`, `utime.py`, `_thinkcad.py` and
`_thinkcad_config.py` are reserved at the project root. Sketch files are bounded
to 100 files / 2 MiB; the interpreter has a 4 MiB heap. A busy sketch must call
delay/yield equivalents, just like a C++ sketch, to avoid the response watchdog.

Tracebacks retain source filenames and line numbers. GDB source breakpoints
remain a C++ feature. Reset restarts the board sketches through ThinkCad's
existing simulation controls. Export `.py` files for transfer with a separate
MicroPython deployment tool; Arduino CLI upload is not used for Python.

## General local Python 3 scripts

Use **Tools / Python 3 scripts** or the same entry in the home navigation. This
uses CPython rather than the board simulator and offers Open, Save, Run, Stop
and a bounded UTF-8 output panel. Sibling imports work from the original script
folder. Unsaved edits run from a temporary copy and are not written over the
original file. These scripts do not implicitly control the simulated boards.

The Windows installer bundles pinned CPython 3.13.16, including its license.
For Linux, install Python 3 using the distribution package manager. Gentoo's
`python` USE flag installs this optional runtime. A custom interpreter can be
selected under Settings / Editor; choose a separately installed Python to use
its additional packages. The embedded Windows runtime does not include pip.

Scripts and libraries execute with your user permissions, as existing native
C++ sketches do. Separate processes do not constitute a security sandbox.

## Project compatibility

Project format 6 stores the language separately for every board. Formats 1–5
still load as Arduino C++. Older ThinkCad versions cannot interpret the new
format. LAN collaboration and copied boards preserve language metadata; received
code is never started automatically.
