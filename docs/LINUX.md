# ThinkCad auf Linux (ab 0.4.1)

Unterstützt werden Windows und Linux; macOS ist im CMake-Build explizit ausgeschlossen.
Die gleiche Qt-Oberfläche, Schaltungsmodelle, `.ino`-Projekte, Library Manager,
Flash-Adapter und Display-API laufen unter Linux. X11 und Wayland werden über Qt
unterstützt. Die bestehenden Simulationsgrenzen gelten weiterhin.

## Binärpakete

Die Release-Seite enthält ein DEB für Ubuntu 24.04 und kompatible Distributionen,
ein RPM für Fedora 44 sowie einen Linux-Tarball aus dem Ubuntu-Build.
Die Pakete verwenden **System-Qt und System-GCC**, keine eingebettete Windows-Toolchain.
Der Tarball lässt sich entpacken und über `ThinkCad-0.5.0-Linux-x86_64/usr/bin/thinkcad`
starten. Er ist kein distributionsunabhängiges AppImage; er benötigt kompatible
Systembibliotheken wie das Ubuntu-Paket. Andere Distributionen können aus Quellen bauen.

```sh
# Ubuntu 24.04 / kompatible Systeme
sudo apt install ./ThinkCad-0.5.0-Linux-x86_64.deb
# Fedora 44
sudo dnf install ./ThinkCad-0.5.0-Fedora44-x86_64.rpm
thinkcad
```

Der Desktop-Eintrag erscheint im Anwendungsmenü. ThinkCad öffnet `.ino`, `.thinkcad`
und ältere `.thinkercad`-Dateien; `thinkcad /pfad/projekt.ino` funktioniert ebenfalls.
Einstellungen, Projekte und installierte Bibliotheken werden außerhalb von `/usr`
im Nutzerverzeichnis gespeichert. Auf HiDPI-Bildschirmen übernimmt Qt die Skalierung.

## Aus Quellen bauen

CMake ≥3.24, ein C++20-Compiler, Qt ≥6.4 mit Widgets/Network und zlib werden benötigt.
Zur Laufzeit braucht die Sketch-Ausführung einen aktiven GCC/g++ mit C++20-Unterstützung
(empfohlen ≥12). Die GUI selbst kann auch mit Clang gebaut werden. Die Bibliothekstests
brauchen Qt Test, für X11 einen Qt-XCB-Plugin und für Wayland den Qt-Wayland-Plugin.

```sh
# Ubuntu 24.04
sudo apt install cmake ninja-build g++ qt6-base-dev qt6-base-dev-tools \
  qt6-qpa-plugins qt6-wayland zlib1g-dev
# Im entpackten Source-Verzeichnis:
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
sudo cmake --install build
thinkcad
```

Für eine Installation ohne Administratorrechte `CMAKE_INSTALL_PREFIX=$HOME/.local`
verwenden und ohne sudo installieren. Bei `DESTDIR` bleibt der konfigurierte Prefix
Teil des Installationsbaums. SDK und Beispiele werden relativ zum installierten
Programm gefunden; der Quellbaum wird zur Laufzeit nicht benötigt.

Mit `QT_QPA_PLATFORM=xcb thinkcad` bzw. `QT_QPA_PLATFORM=wayland thinkcad` kann das
Backend gezielt gewählt werden. Der normale Start verwendet die Qt-Erkennung.
`QT_QPA_PLATFORM=offscreen thinkcad --smoke-test` prüft Compiler, IPC und OLED-Frames
bei installierten Beispielen. Die Testsuite läuft ohne Desktop ebenfalls offscreen.

## Gentoo: Portage und USE-Flags

Das Release enthält **ThinkCad-0.5.0-Gentoo-overlay.tar.xz** mit Ebuild und passendem
DIST-Manifest. Es ist ein lokales Overlay, kein Eintrag im offiziellen Gentoo-Repository.
Das Quellarchiv wird von Portage aus demselben GitHub-Release geladen und geprüft.

```sh
sudo mkdir -p /var/db/repos/thinkcad
sudo tar -xJf ThinkCad-0.5.0-Gentoo-overlay.tar.xz \
  -C /var/db/repos/thinkcad --strip-components=1
sudo mkdir -p /etc/portage/repos.conf
sudo tee /etc/portage/repos.conf/thinkcad.conf <<'CONF'
[thinkcad]
location = /var/db/repos/thinkcad
masters = gentoo
auto-sync = no
CONF
sudo mkdir -p /etc/portage/package.accept_keywords /etc/portage/package.use \
  /etc/portage/package.license
printf 'media-gfx/thinkcad ~amd64\n' | sudo tee /etc/portage/package.accept_keywords/thinkcad
printf 'media-gfx/thinkcad examples test X wayland\n' | sudo tee /etc/portage/package.use/thinkcad
printf 'media-gfx/thinkcad all-rights-reserved\n' | sudo tee /etc/portage/package.license/thinkcad
sudo emerge --ask media-gfx/thinkcad
```

| USE-Flag | Wirkung |
| --- | --- |
| `examples` (Standard an) | Installiert die fertigen Beispielschaltungen und Sketches |
| `test` (Standard aus) | Baut und führt die Testsuite über Portage aus |
| `X` (Standard an) | Fordert Qt mit X11/XCB-Unterstützung an |
| `wayland` (Standard an) | Fordert die Qt-Wayland-Abhängigkeiten an |

Mindestens `X` oder `wayland` muss aktiv sein. `-examples` entfernt die Beispielsketches;
eigene Projekte und die Simulation bleiben nutzbar. Bereits systemweit vorhandene
Qt-Backends werden durch das Abschalten eines Flags nicht entfernt. Die Flags steuern
die Paketabhängigkeiten und optionalen Build-/Installationsschritte. USE-Flags sind
**ausschließlich Gentoo-spezifisch**; DEB/RPM verwenden keine USE-Konfiguration.
Das Ebuild verwendet den aktuellen Gentoo-Qt-Aufbau ab Qt 6.11, bei dem der
Wayland-Client im Qtbase-Paket liegt. Es ist zunächst für `~amd64` vorgesehen. Weitere Architekturen sind noch
nicht durch einen nativen Gentoo-Build validiert.

Die Hauptanwendung hat bislang keine freie Lizenzfreigabe; das Ebuild übernimmt dafür
`all-rights-reserved`, für den Adafruit-Zeichenkern BSD. Es vergibt keine neue Lizenz
für den bestehenden Anwendungscode. Deshalb ist die explizite Gentoo-Lizenzakzeptanz nötig.

Die Dateien unter `packaging/gentoo` sind die Overlay-Vorlagen. Das vollständige
Release-Overlay enthält zusätzlich das zum veröffentlichten Source-Archiv passende
Manifest. Nach jeder neuen Version muss das lokale Overlay aktualisiert werden.

## Updates

Linux verwendet die Paketverwaltung. Die App meldet neuere GitHub-Versionen, ersetzt
aber keine root-eigenen Dateien durch einen Windows-Installer. Neue DEB-/RPM-Pakete
mit apt/dnf installieren; für Gentoo den neuen Overlay-Stand einspielen und
`emerge --ask --oneshot media-gfx/thinkcad` ausführen. `emerge --sync` aktualisiert
das hier manuell eingespielte Overlay mit `auto-sync=no` nicht automatisch.
Der Windows-Updater bleibt unverändert verfügbar.

Die Linux-CI prüft native Ubuntu-/Fedora-Builds, Tests, X11/Wayland, Installation und
Entfernung sowie reale Portage-Builds mit unterschiedlichen USE-Konfigurationen.
Checksummen und GitHub-Build-Attestierungen werden mit den Release-Paketen veröffentlicht.
