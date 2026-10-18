# Simulation inspector

View → Simulation inspector opens the existing-style native tools. The serial
monitor and logic analyzer remain available in their original docks.

- Pins & nets shows solved voltages, digital thresholds and modeled GPIO
  input/output/PWM state. Select a pin to see its electrically connected members
  and recent voltage/logic changes at actual virtual-time samples. Floating or
  indeterminate values stay undefined. Acquisition retains up to 2048 changes;
  the table shows the latest 128. The logic analyzer provides waveform/decoding.
- Device registers shows the selected modeled I2C/SPI module's actual 256-byte
  bank after initialization. Inspection never initializes a device, advances its
  register pointer or clears status bits. An uninitialized device shows dashes.
  These are peripheral models, distinct from the source debugger's PC registers.
- Bus traffic captures actual raw Wire/I2C, SPI and HardwareSerial transactions.
  I2C includes address, ACK/status and START/repeated START/STOP; SPI includes
  mode, bit order and reply status. UART TX is a queued write, RX is a consumed
  received byte; timestamps are API transaction times, not bit waveforms. Legacy
  direct display API frame uploads are not fabricated as raw I2C byte traffic.
- UART state shows configured pins, baud, frame format and actual framing,
  overrun and collision counters.
- Environment / sensors selects module controls independently of canvas
  selection, including while the simulation locks wiring. Temperature, humidity,
  distance and other inputs can change; I2C address and strip geometry remain
  locked while running.

Capture is off when the inspector is closed. While open, the capture checkbox
can stop recording without deleting history. Bus history is bounded to 1024
entries by default (Settings → Performance: 128–4096). Payload retention is capped
at 128 bytes per direction and records the full original byte count. Older entries
are overwritten and counted. CSV exports exactly the retained transaction data.
The UI refreshes at most five times per second; acquisition does not rebuild
widgets for every packet. Starting/resetting a simulation clears acquisition.

External TX/RX loopback wiring is supported, including the same hardware UART.
There is no implicit loopback without a physical connection.
