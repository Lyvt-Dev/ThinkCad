# Audio- und Bauteilerweiterung in 1.3.3

Dieses Dokument beschreibt den geprüften Funktionsumfang und die offenen Modellgrenzen. Die Aufnahme eines Bauteils in die Suche
bedeutet keine vollständige oder datenblattgenaue Emulation. Vor einer Veröffentlichung
müssen insbesondere die unten aufgeführten offenen Prüfungen abgeschlossen werden.

## Bedienen und anschließen

Alle Module befinden sich in der Komponentensuche. Anschlussnamen werden beim
Überfahren eines Pins angezeigt. Versorgung und Masse müssen tatsächlich verbunden
sein; eine installierte Arduino-Bibliothek ersetzt keine angeschlossene Hardware.

Das Breadboard bietet 170, 300 oder 630 Steckkontakte und unabhängig zuschaltbare
Stromschienen. Kontakte a–e und f–j sind pro Spalte verbunden, über den Mittelkanal
hinweg getrennt. Die vier Stromschienen sind voneinander getrennt. Beim Verkleinern
werden Leitungen zu entfernten Kontakten entfernt; Rückgängig stellt sie wieder her.
Die LED-Farbe und Tasterkappenfarbe lassen sich in den Eigenschaften wählen.

### DACs und DDS

- **MCP4725:** VCC/GND, SDA/SCL und VOUT. Standardadresse 0x60,
  acht Adressvarianten. `Adafruit_MCP4725` ist enthalten. Code 2048 liefert bei
  5 V Versorgung unbelastet 2,5 V; Vollaussteuerung ist 4095/4096 der Versorgung.
- **MCP4822/MCP4922:** SPI, CS und LDAC; 16-Bit-Kommandos enthalten Kanal,
  Verstärkung, Abschaltung und 12-Bit-Code. MCP4822 hat eine 2,048-V-Referenz,
  MCP4922 benötigt VREFA/VREFB. LDAC auf GND aktualisiert unmittelbar.
- **AD9833:** SPI-Modus 2; Kontroll-, Frequenz- und Phasenregister.
  Die Tuningfrequenz berechnet sich als MCLK × Frequenzwort / 2²⁸.
- **AD9850:** 40 serielle Bits, LSB zuerst: vier Frequenzbytes, dann Kontrollbyte.
  CS ist die Transaktionsauswahl des Simulators; FQ_UD muss anschließend von
  LOW auf HIGH wechseln, um den neuen Wert zu übernehmen. RESET löscht ihn.
  Frequenz = MCLK × Frequenzwort / 2³². Power-down und 5-Bit-Phase werden berücksichtigt.
- **R-2R:** passives 8-Bit-Netzwerk mit D0 als LSB und D7 als MSB.
  Kein idealer Spannungsausgang: Ausgangswiderstand und gegenseitige Belastung der
  Eingänge werden berücksichtigt. Bei R=10 kΩ, D7=5 V und allen anderen Bits=0 V
  ergeben sich offen 2,5 V, an einer 1-kΩ-Last dagegen ungefähr 0,227 V.

DDS-Register können MHz-Werte enthalten. Das ist keine Zusage einer MHz-genauen
Transientenberechnung. Die Audioausgabe hat 48 kHz; oberhalb von 20 kHz ist das
Signal außerhalb des geprüften Audiobereichs. Es gibt einen Diagnosehinweis.

### I²S und VS1053

PCM5102A benötigt BCLK, LRCK, DIN, Versorgung und einen HIGH-Pegel an XSMT.
WM8731 und CS4272 benötigen zusätzlich die I²C-Konfiguration. Die enthaltenen
`I2S.h`-/`ESP_I2S.h`-Adapter übertragen PCM über die simulierten Verbindungen.
Analoge Codec-Eingänge werden als zeitdiskrete Samples erfasst. DMA, PLL-Jitter,
analoge Rekonstruktionsfilter und sämtliche herstellerspezifischen Registerfunktionen
sind nicht vollständig nachgebildet.

VS1053 hat getrennte SCI-/SDI-Auswahl über CS/XDCS, RESET und DREQ.
Die Simulation dekodiert zugeführte MP3-Daten mit Qt Multimedia. Das ist gepufferte
Host-Dekodierung und keine Emulation des internen DSP-Prozessors. Der Puffer ist
auf 16 MiB codierte Daten begrenzt. Decoder-Verfügbarkeit hängt vom mitgelieferten
Qt-Multimedia-Backend ab. Der generierte 440-Hz-Testclip ist ein eigener Testdatensatz.

Der Echtzeit-MIDI-Pfad verarbeitet Noten, Velocity, Programmwechsel, Lautstärke,
Panorama, Sustain und Pitch Bend. Er erzeugt eigene synthetische Klangfarben;
das General-MIDI-ROM des Originalchips wird nicht nachgebildet. UART-MIDI verwendet
31250 Baud. Optokoppler sind elektrisch vereinfachte Modelle, keine präzisen
Laufzeit- oder Störfestigkeitssimulationen.

### Synthesizer und Effekte

AS3340/CEM3340 liefern Sägezahn, Dreieck und Puls; die Frequenz wird im aktuellen
Modell über eine kalibrierte Grundfrequenz und einen 1-V/Oktave-Steuereingang gesetzt.
Die realen Timing-, Stromsteuerungs- und Temperaturkompensationsschaltungen sind
noch nicht vollständig abgebildet. XR2206/ICL8038 verwenden ebenfalls eine
kalibrierte Grundfrequenz statt einer vollständigen internen Transistorschaltung.

TL072/TL074, NE5532 und OPA2134 berücksichtigen Versorgung, Verstärkung,
Ausgangswiderstand und Begrenzung. Rauschen, Slew Rate, Verzerrungsspektrum und
Temperaturgang sind nicht datenblattgenau. Verstärkermodule sind vereinfachte
Signalmodelle; Verlustleistung, Schutzschaltungen und Class-D-Schaltträger fehlen.

SSI2144 ist ein vereinfachtes Vierpolfilter. PT2399 ist ein zeitdiskreter Delay-Ersatz
mit Rückkopplung und Tiefpass. **FV-1 bietet eigene modellierte Effekte, führt aber
keinen SpinASM-Code und keine Original-ROM-Programme aus.** Diese Modelle dürfen
nicht als Ersatz für die Prüfung eines realen Audioentwurfs beworben werden.

## Noch offene Arbeit vor Freigabe

- Funktionstests für sämtliche hinzugefügten Zähler, Flipflops, Timer, Sensoren,
  Versorgungen, Audiopfade und negative/fehlende Versorgungen.
- Referenzmessungen für Audiofilter, Dynamik, Frequenzwechsel und Verzögerungen.
- Weitere Mikrocontroller-API-Abdeckung, insbesondere micro:bit-Anzeigen/Bedienung.
- Weitere Grenzfälle des Drum-Pad-Impulsmodells, Getriebemodell und mechanische Lasten.
- Durchgängige visuelle Prüfung aller Varianten und ihrer Anschlusspositionen.
- Gentoo-/Windows-Tests der Audio-Backends und des Verhaltens ohne Audiogerät.

Der Arbeitschecklisteneintrag steht in [COMPONENT_UPDATE_WORK.md](COMPONENT_UPDATE_WORK.md).

## Lokale Prüfergebnisse

- Katalogvalidierung einschließlich passiver Module und aller Anschlussnamen.
- 74HC283: alle 512 Kombinationen aus zwei 4-Bit-Eingängen und Carry-in.
- 74HC595: echte Schiebe-/Latch-Sequenz; CD74HC4067: analoger Signalpfad.
- MCP4725/MCP4822: digitale Kommandos bis zur Spannung am realen Schaltungsnetz.
- AD9833/AD9850: Frequenzwörter, LSB-Reihenfolge und FQ_UD-Übernahme.
- R-2R: Halbskala unbelastet und Spannungsabfall unter 1-kΩ-Last.
- TL072/LM741: geschlossene Spannungsfolger mit positiver und negativer Versorgung.
- Drum-Pad: Velocity, Abklingen, Loslassen und erneuter Anschlag.
- MPR121: deaktivierte/aktivierte Elektroden und Soft-Reset; PT2258: Pegel und Mute.
- VS1053: MIDI-Note erzeugt PCM, Note-off endet nach Release; MP3-Testclip ergibt
  440 Hz, weitere Daten liefern zusätzliche Samples ohne Wiederholung des Anfangs.
- 435 elektrische Audio-/PCM-Prüfungen; UI-Farbwahl, Breadboard-Konfiguration,
  Rückgängig/Wiederholen und Anschlusspositionen; 23 Peripheral-Tests bestanden.
- ESP32-S3: das ursprüngliche 20×4-LCD-Sketch mit SDA8/SCL9 und 3,3 V funktioniert
  in Simulation und Emulation.

Dies ist Linux-Cloud-Evidenz. Persönliche Gentoo- und Windows-Tests stehen noch aus.
Die Prüfergebnisse machen die oben beschriebenen Modellgrenzen nicht gegenstandslos.
