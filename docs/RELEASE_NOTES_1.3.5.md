# ThinkCad 1.3.5

## Deutsch

- A4988 und ULN2003 mit 4-/5-/6-Leiter-Schrittmotoren, sichtbarer Welle,
  Schrittmodus, Spulen und Inspektor-Telemetrie. Stepper-/AccelStepper-Kern-APIs
  erzeugen GPIO-Signale; DRIVER-Ziele werden als Sketch-Telemetrie gekennzeichnet.
  Bedienter Endschalter für die Auswertung im Sketch.
- 5-V-SPDT-Relais mit aktiver HIGH-/LOW-Logik, NO/NC-Kontakten, Status-LED und
  optionalem nicht blockierendem Klick.
- ILI9341 und ST7735 mit RGB565, GFX-Text/Grafik/Bitmaps, Rotation und
  SPI-Übertragungszeit. Hoch- und Querformat in 128×160 bzw. 240×320.
- BME680 mit Temperatur, Feuchte, Druck und simuliertem Gaswiderstand; originale
  Adafruit/Bosch-I²C-Treiber. MH-Z19B mit verzögerter UART-CO₂-Antwort und Prüfsumme.
  Werte, Schwankungen und Kommunikationsfehler sind konfigurierbar.
- Lokale komprimierte Share-Codes mit optionalem Argon2id/XChaCha20-Poly1305-
  Passwortschutz. Kleine Projekte als QR-Bild; Import aus Code, Bild oder Kamera,
  mit Vorschau und Bestätigung. Keine Uploads oder automatische Bibliotheksinstallation.
- Bauteilsuche mit Strg+P, sofortigem Suchfokus, Tastaturbedienung, zuletzt/häufig
  verwendeten Bauteilen und rückgängig machbarer Platzierung.
- Freundliche kurze kontextsensitive Quack-Hinweise; neuer Skin **Nyra Quack**.
  Aus/Nur Fehler/Normal/Ausführlich bleiben frei wählbar, ohne Überlagerungen.
- Zehn vollständige verdrahtete Beispiele, aktualisierte Sprachkataloge und
  Modell-/Codec-/GUI-/Compiler-Integrationstests.

Projektformat bleibt 8; bestehende Projekte bleiben lesbar. Kein Login, Cloud-Zwang,
Electron oder macOS. Windows und Linux einschließlich Gentoo bleiben die Plattformen.

**Modellgrenzen:** keine mechanische Last/Resonanz/Schrittverlust- oder vollständige
A4988-Stromregelung; keine chemische Gasanalyse/BSEC; TFT-Controller und SDKs sind
klar begrenzte Teilmodelle. Reine GPIO-Schritte haben kein beobachtbares Ziel.
Kamera und hörbarer Relaisklang benötigen zusätzlich reale Hardwareprüfung.
[Anschluss und APIs](FEATURES_1.3.5.md) · [Share-Code-Sicherheit](SHARE_CODE.md)

## English

- A4988/ULN2003 drivers and 4/5/6-wire steppers, visible shafts, coil/mode/rate
  telemetry, Stepper and core AccelStepper APIs, and a manually operated endstop.
  AccelStepper DRIVER targets are explicit sketch telemetry, not inferred mechanics.
- Powered 5 V SPDT relay with active-high/low input, NO/NC contacts, status LED and
  optional asynchronous click.
- ILI9341/ST7735 RGB565 SPI displays, GFX text/shapes/bitmaps, rotation, scrolling
  register support and simulated transfer time. Portrait and landscape resolutions.
- BME680 forced measurements through original Adafruit/Bosch drivers; simulated
  environment/gas resistance. MH-Z19B delayed CO2 UART replies with checksums.
  Inspector values, optional variation and communication fault injection.
- Local compressed share codes, optional Argon2id/XChaCha20-Poly1305 encryption,
  QR for small projects, image/camera scanning and explicit import preview.
  No uploads, accounts, automatic sketch execution or library installation.
- Ctrl+P component finder with immediate focus, keyboard selection, recent/frequent
  entries and undoable insertion through the existing palette.
- Concise contextual assistant hints and **Nyra Quack** skin; all four assistant modes
  remain available, without covering the workspace.
- Ten fully wired examples and additional model, codec, UI and compiler integration tests.

Project format stays at 8. Existing projects and native Windows/Linux/Gentoo support
remain. No full mechanical dynamics, chemical gas model/BSEC, CPU emulation or
unrestricted display/controller compatibility is claimed. See the linked usage and
model-limit documentation. Physical camera and audio playback require hardware checks.
