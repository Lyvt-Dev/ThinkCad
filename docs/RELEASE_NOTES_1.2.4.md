# ThinkCad 1.2.4

- Kabel werden über Bauteilen und Breadboards dargestellt.
- Direktkontakte ohne Kabel: Einrasten, Platzierungsvorschau, Trennen beim Wegziehen, Speichern und Rückgängig/Wiederholen.
- Einheitlicher physischer Maßstab in der realistischen Ansicht; gleiche LED-Anschlusshöhe mit Knick an der Anode und korrigierte Header-Raster.
- Zusätzliches Breadboard mit 400 Kontakten und vier getrennten Stromschienen.
- 4×4-Keypad, musikalisches 25-Tasten-Keyboard und Drehencoder mit elektrischen Kontakten und beweglichen Bedienfenstern.
- Serieller MIDI-Monitor und Übersetzer, SDK-Adapter für MIDI, Keypad und Encoder sowie drei ausführbare Beispiele.
- Veränderliche Quellen und Regler sind während der Simulation über separate Wertefenster bedienbar.
- Piezo-Eigenschaften öffnen ein 88-Tasten-Piano mit Frequenzanzeige und tone()-Wert.

## Modellgrenzen und Prüfung

Die Keyboard-Komponente ist eine musikalische Matrix, keine USB-PC-Tastatur. MIDI verwendet serielle Daten; native Betriebssystem-MIDI-Ports und USB-MIDI-Emulation sind nicht enthalten. Gehäuse sind repräsentative Bauformen, keine Fertigungs-Footprints. Details: [Montage und MIDI](ASSEMBLY_AND_MIDI.md).

Lokaler Linux-Build und Regressionstests erfolgreich, einschließlich Direktkontakten, Eingaben, MIDI und Audioerzeugung. Die Cloud hat kein Audioausgabegerät; hörbare Ausgabe muss auf dem Zielsystem geprüft werden. Windows- und Linux-Pakete werden separat durch die Release-CI validiert.
