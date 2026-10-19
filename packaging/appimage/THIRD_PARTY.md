# AppImage third-party components

The application uses Qt 6.8.3 (Core, Gui, Widgets, Network, Multimedia, SerialPort
and deployed plugins). LGPL-3.txt is included here; corresponding Qt sources are
published alongside the release as qt-everywhere-src-6.8.3.tar.xz. Users can extract
the AppImage and replace its shared Qt libraries; the AppRun launcher does not
prevent this. The updater accepts official verified releases only.

The bundled Zig 0.14.1 compiler and its C/C++ runtime sources and license notices
are retained under usr/bin/toolchain (LICENSE and lib/). It compiles native Linux
sketches and does not emulate microcontroller CPUs. GitHub CLI 2.102.0 is bundled
for attestation verification; its MIT license is in usr/bin/verifier.

linuxdeploy deploys shared dependencies from Ubuntu 22.04 and copies available
package copyright notices. The AppImage deliberately does not bundle glibc or
host-specific GPU drivers. Desktop and hardware integration still uses the host.

Adafruit-GFX font notices are installed in the application license directory.
Application ownership and licensing remain as stated in the repository license.

ThinkCad 1.3.5 also bundles Monocypher 4.0.2 (BSD-2-Clause / CC0), Nayuki
qrcodegen (MIT), quirc 1.2 (ISC) and Adafruit/Bosch BME680 drivers (BSD).
Their pinned source provenance and full copyright/license notices are preserved
in the source tree and installed licenses directory.
