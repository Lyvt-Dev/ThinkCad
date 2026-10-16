# Local source debugging

Normal simulation needs the existing native C++ compiler only. Source debugging
also needs a local GNU GDB with MI support. The Windows package includes GDB next
to the bundled compiler; Linux users can install their distribution's `gdb`.
Settings → Editor / Code can select a different local GDB executable. No login,
symbol server or remote debugging service is used.

Open View → Source debugger (Quellcode-Debugger). Before starting, click a source
line's gutter to toggle a breakpoint. Breakpoints belong to the source file and
board, follow inserted text, and are saved in `.thinkcad` projects. Debug starts
all board sketches with their own GDB process. Continue resumes them; Next line,
Step into and Step out act on the selected stopped board. Interrupt stops a
running native sketch. Stop terminates the sketches and their debugger processes.

A stopped source location switches the editor to its board and original file.
Other boards pause at their next virtual-time barrier while stepping the stopped
board. This is cooperative simulation scheduling, not synchronized MCU clocks.
During debugging the source is read-only: changing it would invalidate the
compiled source locations. Stop, edit and start debugging again to apply changes.

The variables view shows actual native stack variables. **Host CPU registers**
are the PC's registers, not ESP/AVR/ARM registers. Emulation models peripherals and
bus timing; it does not execute an MCU instruction set. Board and peripheral
state must be inspected separately. Optimized external libraries may have fewer
available variables; sketches are compiled with debug information and `-O0`.

GDB/MI has its own process channel. Debug sketches communicate with the simulator
through a separate authenticated TCP connection bound to loopback, opened only
for an explicitly started debug run and closed on Stop. Normal simulation keeps
its stdin/stdout protocol and opens no debug listener. This process separation
is not a sandbox for untrusted native code or libraries.
