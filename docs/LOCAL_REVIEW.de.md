# ThinkCad 1.0.0 aus Quellen bauen

Das Quellpaket enthält SDK, Beispiele und Screenshots. Es enthält keine fertigen
Windows-Binärdateien; diese stehen im GitHub-Release bereit. Für Gentoo kann das
passende Release-Overlay verwendet werden (siehe README). Die folgenden Schritte
bauen direkt aus entpackten Quellen. Eine bestehende Installation kann parallel bleiben.

## Gentoo

Diese Abhängigkeiten bei Bedarf als root installieren. Bei bereits funktionierendem
ThinkCad-Build ist vor allem Qt SerialPort zusätzlich nötig. Portage zeigt fehlende
USE-Flags oder Keywords für die konkrete Gentoo-Konfiguration an.

```sh
emerge --noreplace --ask \
  '>=sys-devel/gcc-12[cxx]' '>=dev-build/cmake-3.24' dev-build/ninja \
  '>=dev-qt/qtbase-6.4:6[gui,network,ssl,widgets]' \
  '>=dev-qt/qtmultimedia-6.4:6' '>=dev-qt/qtserialport-6.4:6' \
  sys-libs/zlib media-fonts/dejavu
```

ZIP entpacken, ein Terminal im entpackten Ordner `ThinkCad` öffnen und als
normaler Benutzer ausführen:

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure --parallel 2
./build/thinkcad
```

Die letzte Zeile benötigt eine laufende grafische X11-/Wayland-Sitzung. Der Build
kopiert SDK, Beispiele und Handbuch neben die Anwendung; eine Installation oder
Änderung des Overlays ist zum Testen nicht nötig. Nicht den alten Menüeintrag des
installierten Pakets starten. Für Sketches muss `g++` verfügbar sein.

## Windows

Siehe [Windows bauen](../README.md#windows-bauen): VS2022-C++-Tools, CMake/Ninja,
Qt Widgets/Network/Multimedia/SerialPort und Zlib für die GUI, zusätzlich
MinGW-w64/UCRT64 `g++.exe` mit Standardbibliotheken für die Sketch-Ausführung.
Qt-Kit und Compiler müssen zur Architektur passen. Die GitHub-Workflowdatei
`.github/workflows/windows.yml` enthält die vollständigen Abhängigkeiten und
Deployment-Schritte; sie wurde durch dieses Quellpaket nicht ausgelöst.

Echte Firmware-Uploads benötigen Arduino CLI, Board-Core, Originalbibliotheken
und den richtigen COM-/tty-Port. Die neuen Modulbeispiele und Details stehen im
[Modulhandbuch](WIRELESS_MODULES.de.md); die [Screenshot-Galerie](V1_REVIEW.md)
zeigt den hier auf Linux geprüften Stand.
