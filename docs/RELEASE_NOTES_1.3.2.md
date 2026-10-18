# ThinkCad 1.3.2 – Quak-Assistent

Ein kleines, bewusst verspieltes Update: Die pixelige Diagnose-Ente begleitet die vorhandene elektrische Diagnose.

- **Hilfe → Quak-Assistent** öffnet den bestehenden Diagnose-Dock und die Ente.
- Eine ausgewählte Diagnosemeldung erscheint mit kurzer Quak-Einleitung, Erklärung und konkretem Lösungsvorschlag.
- **Stelle zeigen** wählt das betroffene Bauteil, den Pin oder das Kabel aus. **Nächster Hinweis** wechselt zwischen den sichtbaren Meldungen.
- Der Assistent lässt sich direkt im Dock ausblenden; die Auswahl bleibt lokal gespeichert. Der Hilfeeintrag blendet ihn wieder ein.
- Texte sind in allen acht vorhandenen Oberflächensprachen ergänzt. Das transparente Pixelmotiv funktioniert im Hell- und Dunkelmodus; Bedienelemente bleiben native Qt-Steuerelemente.

## Ausprobieren

1. **Datei → Beispielschaltungen → Blink** öffnen.
2. Bei gestoppter Simulation den LED-Vorwiderstand entfernen und die LED direkt mit dem Ausgang verbinden.
3. **Hilfe → Quak-Assistent** öffnen, Diagnose auswählen und **Stelle zeigen** betätigen.
4. Den Vorwiderstand wieder einsetzen und die Diagnose erneut prüfen.

Der Assistent verwendet dieselben tatsächlichen Diagnosebefunde wie die bisherige Liste. Er ist regelbasiert und vollständig lokal: kein KI-Dienst, kein Login und kein Upload. Ohne Meldungen wird ausdrücklich keine Garantie für reale Hardware gegeben. Die Filter der Diagnoseliste gelten auch für die Ente. Es werden keine neuen Hardwareeigenschaften oder allgemeinen C++-Ursachenanalysen behauptet.

Das Projektformat bleibt **8**, vorhandene Schaltungen, Sketches, Simulationsmodelle und die normale Editoroberfläche bleiben kompatibel. Windows und Linux einschließlich Gentoo bleiben unterstützt.

[Weiterentwicklungsplan für 1.4.0–2.0.0](development/1.3.2-roadmap.de.md)
