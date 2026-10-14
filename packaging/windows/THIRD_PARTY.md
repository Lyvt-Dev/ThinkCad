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
compiler itself. The release workflow collects source packages matching the installed compiler,
binutils and dependency versions into ThinkerCad-Toolchain-Sources.tar. It also
publishes Qt 6.8.3 corresponding sources alongside the installer. Keep these release
assets available with the installer.
MSYS2 package source recipes: https://github.com/msys2/MINGW-packages.
Recipes alone do not replace the complete corresponding-source obligation.

The simulator's Arduino, Wire and SSD1306 headers are small compatibility adapters
written for this project. They do not contain the original Adafruit libraries.
Externally added libraries retain their respective licenses.

Package versions and source hashes are included in the toolchain source archive.
The project's own source currently has no additional license grant; third-party
licenses and user rights for Qt/toolchain components remain as described above. Code signing and an installer do not settle licensing.
