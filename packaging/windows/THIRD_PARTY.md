# Distribution notices

ThinkerCad dynamically links Qt 6 (Qt Core, Gui, Widgets, Network and deployed plugins).
Qt is copyright The Qt Company Ltd. and its contributors. The selected open-source
modules are available under LGPL v3; a copy is bundled as LGPL-3.txt. See
https://www.qt.io/licensing/open-source-lgpl-obligations and the exact 6.8.3
sources at https://download.qt.io/archive/qt/6.8/6.8.3/single/.
Users may replace the Qt DLLs with compatible modified builds. Do not prohibit
reverse engineering for debugging modifications to these libraries.

The Windows installer also includes the MSYS2 UCRT64 GCC toolchain. Preserve its
share/licenses directory. GCC and binutils have GPL licenses; the GCC runtime
exception applies to qualifying runtime linkage, not to distribution of the
compiler itself. Before publicly distributing installers, provide corresponding
source for the EXACT compiler/binutils packages and all other copyleft packages
you bundle, with a durable source-download location or a valid source offer.
MSYS2 package source recipes: https://github.com/msys2/MINGW-packages.
Recipes alone do not replace the complete corresponding-source obligation.

The simulator's Arduino, Wire and SSD1306 headers are small compatibility adapters
written for this project. They do not contain the original Adafruit libraries.
Externally added libraries retain their respective licenses.

Release checklist: record exact binary package versions, archive corresponding
sources, include each license/notice, and define the project's own license before
public distribution. Code signing and an installer do not settle licensing.
