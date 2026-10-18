# ThinkCad 1.3.3 – Transparente lokale Updates

## Neue Funktionen

- Nativer Update-Dialog unter **Hilfe → Nach Updates suchen**: aktuelle und angebotene Version, Veröffentlichungsdatum, Release-Notes, Downloadgröße und Prüfstatus.
- Updates werden nach einer verfügbaren Version zunächst angeboten, statt ohne Nachfrage das gesamte Paket herunterzuladen. Automatische Prüfungen lassen sich abschalten; einzelne Versionen überspringen.
- Echter Downloadfortschritt mit MiB, Prozent, gemessener Geschwindigkeit und geschätzter Restzeit. Pause, Abbruch mit oder ohne Aufbewahren der Teildatei sowie Fortsetzung nach erneutem Start sind möglich.
- Range-Fortsetzung mit geprüftem Content-Range. Ignoriert der Server Range, startet der Download von vorn. Bei temporären Verbindungsfehlern gibt es höchstens drei automatische Wiederholungen mit Wartezeit.
- Separater sichtbarer Status für SHA-256 und GitHub-/Sigstore-Attestierung. Installation erst nach beiden Prüfungen; Repository, Workflow, main-Branch und Quellcommit bleiben verbindlich.
- Vor Installation: Zusammenfassung und Bestätigung. Laufende Simulation stoppen, alle ungespeicherten Änderungen einschließlich Board-Sketches speichern; Abbruch des Speicherns startet keinen Installer.

## Windows-Installation und Wiederherstellung

Ein Qt-freier Helfer außerhalb der Installation wartet auf das Beenden von ThinkCad und prüft den Buildnachweis erneut. Er sichert das bisherige Programmverzeichnis durch Umbenennen und kopiert dessen Dateien zurück in das Arbeitsverzeichnis. Damit bleiben auch zusätzliche lokale Dateien erhalten, bevor der verifizierte Inno-Installer neue Programmdateien schreibt.

Die Oberfläche zeigt Vorbereitung, Sicherung mit echtem Kopierfortschritt, Entpacken/Ersetzen mit dem tatsächlichen Inno-Fortschritt, Startprüfung, Konfiguration und Abschluss. Entpacken und Ersetzen bilden eine gemeinsame Phase; dafür werden keine erfundenen Einzelprozente angezeigt. Während Prüfungen ohne messbare Gesamtschritte bleibt der Balken unbestimmt.

Installerfehler, Abbruch oder fehlgeschlagene Startprüfung stellen die Sicherung wieder her, soweit Dateisystem und Zugriffsrechte das zulassen. Neue fehlerhafte Dateien bleiben getrennt erhalten. Ein vorab gespeichertes Journal und ein temporärer HKCU-RunOnce-Eintrag ermöglichen Wiederherstellung nach unterbrochener Installation beim nächsten Windows-Anmelden. Nach erfolgreichem Abschluss wird dieser Eintrag entfernt.

Die neue Anwendung muss ihren erwarteten Versionsstand bestätigen und den vorhandenen Smoke-Test bestehen. Die vorherige Installation bleibt anschließend als `.thinkcad-backup-*` neben dem Programmverzeichnis erhalten. Diese Sicherungen werden bewusst nicht automatisch gelöscht; sie benötigen zusätzlichen Speicherplatz. Der Dialog weist bei Platz-/Schreibfehlern auf den betroffenen Schritt hin.

## Linux

DEB/RPM und Gentoo bleiben vollständig unter Kontrolle der Paketverwaltung. Der Dialog zeigt das verfügbare Release und die passende Aktualisierungsanweisung.

Ein benutzereigenes x86_64-AppImage kann direkt aktualisiert werden: verifizierter Download, Sicherung als `.previous`, atomarer Austausch und Startprüfung. Bei fehlgeschlagener Startprüfung wird die Sicherung zurückgeschrieben. Für einen Stromausfall nach dem atomaren Austausch steht die `.previous`-Datei zur manuellen Wiederherstellung bereit; es wird keine vollständige Linux-Transaktionsverwaltung behauptet.

## Protokolle, Einstellungen und Grenzen

- Download-/Prüfprotokoll lokal über den Dialog öffnen. Windows speichert zusätzlich `installation.log`, `installer.log` und das Transaktionsjournal im jeweiligen Update-Cache.
- Nach erfolgreichem Neustart zeigen die Einstellungen Version, Installationsdatum und Installationspfad des letzten Updates. Eine Zeitschätzung wird nur angeboten, wenn eine tatsächlich gemessene vorherige Installationsdauer vorliegt; beim ersten Update bleibt sie unbekannt.
- Deutsch/Englisch sind redaktionell ergänzt; die sechs weiteren Sprachkataloge verwenden lokal erzeugte, auf Platzhalter und kritische Texte geprüfte Übersetzungen. Native Windows-Helfertexte stehen auf Deutsch/Englisch zur Verfügung.
- Das Update übernimmt keine Kontrolle über Projekte außerhalb des Programmverzeichnisses, lokale Bibliotheksordner, Skriptverzeichnisse oder Wiederherstellungspunkte. Diese werden nicht gelöscht oder migriert.
- Kein Login, keine Cloudpflicht, kein Electron, keine neue Laufzeitabhängigkeit. Keine zusätzlichen Elektronikfunktionen in diesem Update.
- Der neue Updater ist **ab installierter Version 1.3.3** aktiv. Der erste Download von 1.3.2 auf 1.3.3 erfolgt zwangsläufig noch mit dem alten Updater.

## Projektformat

Unverändert **8**, weiterhin Lesen der Formate 1–8. Keine Projektformatmigration erforderlich.

[Technischer Ablauf und Modellgrenzen](UPDATER.md)
