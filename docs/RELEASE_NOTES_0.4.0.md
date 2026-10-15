# ThinkCad 0.4.0

ThinkerCad heißt jetzt **ThinkCad**. Fenster, Programmdatei, Installer und neue
`.thinkcad`-Projekte verwenden den neuen Namen. Alte `.thinkercad`-Projekte und
bisherige Einstellungen bleiben verwendbar.

- Frei gesetzte Kabelknicke, nachträglich verschiebbare Knicke und Undo/Redo.
- Dark Mode, lesbare weiße Menütexte auf dunklem Hintergrund und Tutorial unter Hilfe.
- Offizieller Arduino-Bibliothekskatalog mit Suche, geprüften ZIP-Downloads und
  Abhängigkeiten. Nur tatsächlich verwendete C++-Bibliotheken werden kompiliert.
- Originaler Adafruit-GFX-Zeichenkern einschließlich `setCursor`, `setTextColor`,
  Text, Schriften, Rotation und Zeichenprimitiven; SSD1306 mit 128×32 oder 128×64.
- ESP32- und ESP32-S3-Boardprofile mit angepassten GPIOs und ADC-Grenzen.
- Erweiterter Katalog mit insgesamt 20 Typen, darunter Breadboard, RGB-LED,
  Siebensegmentanzeige, Potentiometer, LDR, NTC, Schalter und Spannungsquellen.
- Automatische Updates prüfen GitHub/Sigstore-Build-Attestierungen anstelle eines
  verpflichtenden Herausgeberzertifikats und installieren nach Neustart.

**Installation von 0.3.0:** Einmal den neuen Installer manuell starten, weil der alte
Client automatische Updates deaktiviert hatte. Nachfolgende Updates können direkt
in ThinkCad installiert werden.

**Grenzen:** Noch keine vollständige Tinkercad-Bauteilabdeckung oder ESP-CPU-Emulation.
Motoren, komplexe ICs, dynamische SPICE-Modelle und weitere Peripherie fehlen.
Hardwarebibliotheken benötigen passende Simulatoradapter; der Onlinekatalog allein
macht sie nicht hardwarekompatibel. Details: docs/COMPONENT_SUPPORT.md im Repository.
