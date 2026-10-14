# ThinkerCad

Native 2D-Desktop-Anwendung in **C++20 und Qt 6**, mit Windows als Zielplattform.
Version 0.2.0 bietet einen frei verdrahtbaren ESP8266-Arbeitsbereich, SSD1306-OLED,
LEDs, Widerstände, Taster, einen Sketch-Editor und eine compilerbasierte Arduino-API-Teilmenge.

![Native Qt-Oberfläche im getesteten Demo-Aufbau](docs/desktop-preview.png)

## Was bereits funktioniert

- Normale Windows-Titelleiste mit Schließen, Minimieren, Maximieren und Größenänderung.
- Neues Logo, moderne helle Oberfläche, andockbare Bauteil-, Code- und Ausgabefenster.
- Bauteile verschieben, Pins durch Kabel verbinden, Kabelfarbe, Löschen, Undo/Redo und Zoom.
- `.ino`-Projekte mit weiteren Sketch-Tabs, Headern und C++-Quellen öffnen.
- Schaltung und Code gemeinsam als portable `.thinkercad`-Datei speichern und wieder öffnen.
- Eigener zeitbasierter Simulationskern mit deterministischer Ereignisreihenfolge.
- GPIO-Pinmodi, externe Eingänge, schwebende Pegel und Treiberkonflikte.
- A0 mit 10-Bit-Quantisierung, **0–1 V am nackten ESP8266**; NodeMCU-Spannungsteiler noch nicht modelliert.
- Resistives Gleichspannungsnetz mit schwebenden Netzen, Pullups, LED-Vorwärtsspannung,
  Überstrom- und Kurzschlussdiagnosen; Taster während der Simulation betätigen.
- OLED funktioniert nur mit angeschlossener Versorgung und korrekter I²C-Verkabelung.
- 128×64 monochrome OLED-Frames, Bitmap-Arrays, Bildimport und Animation.
- I²C-Übertragungszeit: etwa 23,085 ms für 1026 Bytes bei 400 kHz.
- Echter C++-Compiler für `setup()` / `loop()` in einem separaten Sketch-Prozess.
- Lokale portable Bibliotheken: Include-Pfade und C++-Quelldateien werden eingebunden.
- Windows-Installerdefinition, Build-Workflow und vorbereiteter GitHub-Release-Updater.

**Aktuelle Grenzen:** Kein Breadboard, kein dynamischer SPICE-Netzlöser, keine
Xtensa-CPU-Emulation und keine vollständige
ESP8266-Arduino-Core-Kompatibilität. Wi-Fi, Timerinterrupts, PWM, UART-Peripherie,
HD44780/LCD1602 und beliebige I²C-Bibliotheken sind noch nicht implementiert.
Die mitgelieferten Adafruit-Header sind ausdrücklich **Simulatoradapter**, nicht die
vollständigen upstream-Bibliotheken. Native Sketches sind nicht sicher sandboxed;
nur eigenen bzw. vertrauenswürdigen Code und Bibliotheken ausführen.

## Windows bauen

Benötigt werden CMake ≥3.24, Qt ≥6.5 (getestet: Linux Qt 6.8.2 und Windows Qt 6.8.3/MSVC2022), Visual Studio 2022 C++ Build Tools und für die Laufzeit-Sketches
MinGW-w64/UCRT64 GCC mit vollständigen Headern/Standardbibliotheken.
Die GUI kann mit MSVC gebaut werden; Sketches laufen als eigenständige MinGW-Prozesse.

In einer **x64 Native Tools Command Prompt**:

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=C:/Qt/6.8.3/msvc2022_64
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
cmake --install build --prefix "${PWD}/out/stage"
```

Für den GUI-Smoke-Test muss `g++` im PATH verfügbar sein. In der App kann unter
**Einstellungen → Sketch-Compiler wählen** ein `g++.exe` ausgewählt werden.
Ein Installer-Build kann die komplette UCRT64-Toolchain nach `out/stage/toolchain`
kopieren; dieser Pfad wird automatisch erkannt. Der CI-Workflow übernimmt dies.

Den Installer mit Inno Setup 6 erstellen:

```powershell
ISCC.exe /DVersion=0.2.0 /DStagingDir=C:\Pfad\ThinkerCad\out\stage packaging/windows/ThinkerCad.iss
```

Er installiert ohne Administratorrechte nach `%LOCALAPPDATA%\Programs\ThinkerCad`,
inklusive Deinstallation und optionaler Desktop-Verknüpfung. Sketches und Einstellungen
liegen außerhalb des Installationsverzeichnisses und werden durch Updates erhalten.

## Kern ohne GUI bauen

```sh
cmake -S . -B build-core -DTHINKERCAD_GUI=OFF -DCMAKE_BUILD_TYPE=Debug
cmake --build build-core --parallel 2
ctest --test-dir build-core --output-on-failure
./build-core/thinkercad_demo
./build-core/sketch_example --iterations 2
```

`core_tests` und `circuit_tests` prüfen Timing, GPIO und elektrische Netzwerke. `sketch_example` kompiliert die echte
Datei `examples/animation.ino` mit dem Simulator-SDK. `desktop_smoke` kompiliert den
Sketch nochmals aus dem GUI-Editor und prüft GPIO, virtuelle Zeit und tatsächliche
OLED-Pixeländerungen. `library_and_ipc` prüft externe Header/Quellen und
GPIO-/ADC-Anfragen mit Host-Antworten. `desktop_interactions` prüft Mausverdrahtung,
Undo/Redo, portable Projekte, `.ino`-Import mit lokalen Quellen, Taster/LED,
fehlende Display-Verbindungen und Schließen während laufender Simulation. Qt verwendet
dafür `QT_QPA_PLATFORM=offscreen`. `windows_native_chrome` prüft unter Windows
die echten Win32-Fensterstile sowie Minimieren, Maximieren, Wiederherstellen und Schließen.

## Bibliotheken und Frames

Unter **Bibliotheken verwalten** einen lokalen Bibliotheksordner hinzufügen.
Arduino-Bibliotheken mit `src/` werden rekursiv kompiliert; ältere Layouts verwenden
Root und `utility/`. `.cpp`, `.cc`, `.cxx` sind unterstützt; C-Dateien, Assembly,
`library.properties`-Abhängigkeitsauflösung und Online-Paketinstallation folgen später.
Bibliotheken, die nur portable C++-Logik und unterstützte APIs brauchen, funktionieren.
Hardwarezugriffe benötigen ein Peripheriemodell und einen expliziten Adapter.

**Bild als Frame-Array einfügen** konvertiert ein Bild zu einem 128×64-Bitmap
(Schwelle 128, MSB zuerst). Die erzeugte globale Deklaration wird an den Sketch-Anfang
gefügt. Den kommentierten `drawBitmap()`-Aufruf in `loop()` übernehmen. Mehrere Arrays
lassen sich wie in `examples/animation.ino` zyklisch abspielen.

## Updates und Releases

Die App fragt GitHub Releases ab, lädt neuere stabile Installer im Hintergrund herunter
und prüft SHA-256 sowie **Windows Authenticode mit fest hinterlegtem Zertifikatsfingerabdruck**.
Erst danach erscheint **Update installieren und neu starten**. Der Installer ersetzt
die Dateien nach dem Beenden und startet die App anschließend wieder; es gibt keinen
manuellen Browser-Download.

Unsigned Entwicklungsbuilds haben automatische Installation bewusst deaktiviert.
Für signierte Releases sind die GitHub-Actions-Secrets `SIGNING_PFX_BASE64` und
`SIGNING_PFX_PASSWORD` nötig: ein vertrauenswürdiges exportierbares Code-Signing-Zertifikat.
Nicht exportierbare Hardware-/Cloud-Zertifikate benötigen eine Anpassung des Signierjobs.
Secrets gehören ausschließlich in GitHub-Einstellungen, niemals in Quellcode/Chat.
Der Workflow leitet den SHA-256-Fingerprint aus dem Zertifikat ab und integriert ihn
in den Build. Zertifikatswechsel brauchen einen geplanten Übergang mit mehreren Pins;
die erste Version unterstützt nur einen. Proxy-Zugriff muss `api.github.com`,
`github.com` und `release-assets.githubusercontent.com` erlauben.

Zum Release die CMake-Projektversion erhöhen und die passenden Release Notes hinzufügen.
Der Windows-Workflow baut/testet/paketiert bei einem Push auf `main`. Nach bestandenem
Build einen passenden `vX.Y.Z`-Tag auf genau diesem Commit erstellen und den Workflow
**Publish verified Windows build** mit Build-Run-ID und Tag starten. Er prüft erfolgreiche
Build-Provenienz, identischen Tag-Commit und Payload-Checksummen, bevor er den bereits
getesteten Installer veröffentlicht. Release-Assets enthalten Installer, SHA-256 und
zugehörige Qt-/Compilerquellen. Unsigned Installer können manuell installiert werden;
automatische Update-Installation bleibt ohne Signierkonfiguration deaktiviert.

## Schaltung und Arduino-Projekt benutzen

1. **Datei → Projekt / .ino öffnen** lädt das Arduino-Projekt als bearbeitbare Kopie.
   Originaldateien werden nicht automatisch überschrieben. Der ordnernamengleiche Sketch
   kommt zuerst, danach weitere `.ino`-Tabs alphabetisch sowie Header und C++-Dateien
   im Projektroot und unter `src/` / `include/`.
2. Links Bauteile hinzufügen und im Arbeitsbereich verschieben. Startpin und Zielpin
   anklicken, um ein Kabel anzulegen. **Esc** bricht ab, **Entf** löscht die Auswahl;
   die mittlere Maustaste verschiebt die Ansicht und **Strg + Mausrad** zoomt.
3. OLED: `VCC → 3V3`, `GND → GND`, `SDA → D2/GPIO4`, `SCL → D1/GPIO5`;
   Sketch: `Wire.begin(D2, D1)` und Displayadresse `0x3C` bzw. `0x3D`.
   Widerstands-/Spannungswerte und Displayadresse stehen in der Bauteil-Auswahl.
4. **Simulation starten** / **F5** kompiliert den Code und startet `setup()` / `loop()`.
   Taster per Doppelklick oder Auswahl-Checkbox betätigen. Topologie erst nach Stop ändern.
5. **Strg+S** speichert Code und Schaltung zusammen. **Arduino-Sketch exportieren**
   schreibt die bearbeiteten Tabs ausdrücklich in einen Arduino-Projektordner.

Einfache globale Funktionen erhalten automatische Vorwärtsdeklarationen. Für komplexe
Templates, Defaultargumente, bedingte Präprozessorblöcke oder spezielle Typdeklarationen
explizite Funktionsprototypen bzw. Header verwenden. Das ist kein vollständiger Ersatz
für den Arduino-Build-Preprocessor. Die unterstützte API-Teilmenge gilt auch für importierte Projekte.

Architektur, Ausführungspfad, Modellgrenzen und Ausbauplan: [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md).
