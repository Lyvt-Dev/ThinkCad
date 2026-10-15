# ThinkCad 0.4.1

Linux ist jetzt neben Windows eine unterstützte Desktop-Plattform. macOS ist
explizit ausgeschlossen und wird weder gebaut noch paketiert.

- Native Linux-Installation mit `thinkcad`, Desktop-Eintrag, Projekt-Dateizuordnung,
  getrennten SDK-/Beispieldaten und systemweitem oder benutzerlokalem Prefix.
- X11 und Wayland über Qt; dieselbe Simulation, `.ino`-Bearbeitung, Bibliotheken
  und Display-APIs wie unter Windows.
- DEB für Ubuntu 24.04, RPM für Fedora 44, Linux-Tarball und Source-Archiv.
- Gentoo-Overlay mit EAPI-8-Ebuild, Manifest und USE-Flags `examples`, `test`, `X`
  und `wayland`. Die Anwendung wird dabei aus Quellen kompiliert.
- Linux-Updates über Paketverwaltung; die App meldet neue Releases. Der Windows-
  Updater wird weiterhin mit dem passenden Windows-Installer ausgeliefert.

Binärpakete benötigen kompatible Systembibliotheken und GCC für die Sketch-Ausführung.
Das Gentoo-Overlay ist lokal einzurichten und bei neuen Versionen zu aktualisieren.
Gentoo zunächst `~amd64`; keine pauschale Zusage für alle Distributionen/Architekturen.
Anleitung: docs/LINUX.md. Bestehende Simulationsgrenzen bleiben bestehen.
