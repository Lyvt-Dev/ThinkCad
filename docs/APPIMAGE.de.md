# Portable AppImage und Updates

Ab 1.2.0 gibt es neben Portage, DEB und RPM eine portable **x86_64-AppImage**.
Sie enthält Qt, Plugins, Ressourcen, Beispiele, Offline-Hilfe, einen Zig/Clang-
basierten C++20-Sketch-Compiler und den Verifizierer für GitHub-Buildnachweise.
Ein systemweites Qt oder g++ ist für die enthaltenen Simulationsbeispiele nicht nötig.
Beim ersten Sketch-Start wird zusätzlich die C++-Standardbibliothek im lokalen
Cache vorbereitet. Das kann je nach Rechner einige Minuten dauern; spätere
Kompilierungen verwenden den Cache.
Echte Firmware-Uploads benötigen weiterhin Arduino CLI, Board-Core, Original-
Bibliotheken und Zugriff auf das USB-Gerät.

## Starten – auch auf Gentoo

Im GitHub-Release `ThinkCad-1.2.2-x86_64.AppImage` herunterladen. Die Datei in einem
vom eigenen Benutzer beschreibbaren Ordner ablegen, zum Beispiel `~/Applications`.
Nach dem Download dort ausführen:

```sh
chmod +x ThinkCad-1.2.2-x86_64.AppImage
./ThinkCad-1.2.2-x86_64.AppImage
```

Falls FUSE fehlt, funktioniert die Extraktionsvariante ohne Root-Rechte:

```sh
APPIMAGE_EXTRACT_AND_RUN=1 ./ThinkCad-1.2.2-x86_64.AppImage
```

Voraussetzungen: x86_64-Linux mit **glibc ≥2.35**, eine grafische X11- oder
Wayland-Sitzung und passende Grafiktreiber. Gebaut wird auf Ubuntu 22.04.
Damit wird keine Unterstützung aller Linux-Systeme versprochen: 32 Bit, ARM,
reine musl-Systeme wie unverändertes Alpine und ältere glibc-Versionen sind nicht
abgedeckt. Moderne glibc-basierte Gentoo-Systeme benötigen kein Overlay für die
AppImage. Audio braucht einen erreichbaren Audio-Dienst des Desktops.

## In-App aktualisieren

ThinkCad sucht nach stabilen Updates. **Hilfe → Nach Updates suchen** prüft sofort.
Bei einer neuen Version wird die passende AppImage im Hintergrund heruntergeladen.
SHA-256, GitHub-Buildnachweis, Herkunfts-Workflow und Release-Commit werden geprüft.
Ein Button **Update installieren und neu starten** erscheint in der Statusleiste
und im Hilfe-Menü. Ungespeicherte Arbeit lässt sich vorher speichern.

Das Update ersetzt die gestartete AppImage atomar an derselben Stelle und startet
sie neu. Ihr Dateiname kann dabei die alte Versionsnummer behalten; die Anwendung
zeigt die installierte Version. Projekte und Einstellungen bleiben erhalten.
Bei Download-/Prüffehlern bleibt die vorhandene Datei bestehen. Das Verzeichnis
und die Datei müssen dem Benutzer das Schreiben erlauben; eine weitere Instanz
kann den Austausch blockieren. Es werden keine Root-Rechte angefordert.

Eine über Portage, DEB oder RPM installierte Kopie wird weiterhin mit ihrer
Paketverwaltung aktualisiert. Wer auf Gentoo die integrierte Aktualisierung
verwenden möchte, startet stattdessen die AppImage. Ein Update aus 1.0.0 kann keine
AppImage-Funktion nachrüsten: dazu die aktuelle AppImage einmal herunterladen.

## Bauen und Prüfen

```sh
docker build -t thinkcad-appimage packaging/appimage
docker run --rm -v "$PWD:/src" thinkcad-appimage
```

Die Ausgabe liegt in `out/appimage/`, einschließlich SHA-256-Datei. Werkzeuge und
Compiler-Archiv werden gegen `packaging/appimage/tools.json` geprüft. Der Build
führt Smoke-Tests am AppDir und an der fertigen AppImage durch. Der Release-
Workflow erzeugt den zur AppImage gehörenden Buildnachweis.

Der Sketch-Compiler erzeugt statische Linux-Sketch-Prozesse mit musl/libc++.
Hardwarebibliotheken funktionieren weiterhin nur mit den dokumentierten
Simulatoradaptern. Bestehende eigene externe Bibliotheken sind mit diesem
Compiler zu prüfen; ein systemweiter Compiler lässt sich in Einstellungen wählen.
