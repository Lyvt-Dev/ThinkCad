# ThinkCad 1.3.6 — Updater fix & Halloween theme

## Deutsch

- Windows-Updater: Deinstallationsregistrierung wird als Datenkopie statt einschließlich Sicherheitsberechtigungen gesichert und wiederhergestellt. Konkrete Windows-Fehlercodes helfen bei der Diagnose.
- Wiederherstellung: Ein fehlgeschlagenes Registry-Backup überschreibt nicht die ursprüngliche Registrierung. Bei gescheiterter Wiederherstellung bleibt das Journal für einen weiteren Versuch erhalten.
- Reparaturinstallation: Eine erfolgreiche manuelle Installation archiviert einen alten ThinkCad-Wiederherstellungsauftrag und entfernt ihn aus RunOnce. Backups, Journale und persönliche Dateien werden nicht gelöscht.
- Neues Halloween-Logo: orangefarbener Chip mit weißer Spinne, einschließlich Windows-Programmsymbol.
- Optionales Halloween-Theme unter **Einstellungen → Allgemein**: dunkles Violett und orange Akzente für Oberfläche, Menüs, Editor und Zeichenfläche. Ausschalten stellt die gespeicherte Hell-/Dunkel-Darstellung wieder her. Speichern und Abbrechen werden berücksichtigt.
- Keine Projektformatänderung. Projekte, Bibliotheken und Einstellungen bleiben lokal erhalten. Windows und Linux einschließlich Gentoo; kein macOS.

**Wichtig für Windows 1.3.5:** Der alte Update-Helfer kann diesen Fix nicht selbst installieren. ThinkCad schließen und den 1.3.6-Installer einmalig manuell über die vorhandene Installation installieren. Nicht vorher deinstallieren. Update-Backups und Journale behalten. Danach verwenden zukünftige Updates den korrigierten Helfer.

## English

- Windows updater: uninstall-registration backup and restore copy data without cloning security descriptors. Failures include the Windows status code.
- Recovery: a failed registry backup cannot overwrite the original registration with its partial snapshot. Failed recovery retains its journal for another attempt.
- Repair installation: a successful manual installation archives and retires an old ThinkCad RunOnce recovery command without deleting backups, journals or personal files.
- New orange chip/white-spider Halloween logo, including the Windows application icon.
- Optional **Settings → General → Halloween theme**: dark purple surfaces and orange accents across menus, editor, canvas and dialogs. Turning it off restores your saved light/dark preference. Save and Cancel are respected.
- No project-format migration. Projects, libraries and settings remain local. Windows and Linux, including Gentoo; no macOS.

**Windows 1.3.5 users:** the old updater helper cannot install its own fix. Close ThinkCad and run the 1.3.6 installer manually over the existing installation once. Do not uninstall first. Keep update backups and journals. Subsequent updates use the corrected helper.
