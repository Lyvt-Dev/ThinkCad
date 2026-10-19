#include "feature135examples.hpp"
#include "language.hpp"
#include <algorithm>
using namespace thinkercad;
QList<CircuitExample> feature135Examples() {
    auto t = [](const char *s) { return Language::text(s); };
    return {
        {"stepper_a4988", t("A4988: Schritte und Richtung"),
         t("Uno mit 12-V-Motorversorgung und getrenntem STEP/DIR. Position im Inspektor verfolgen."),
         t("STEP/DIR und Versorgung verstehen."),
         t("Der Motor bewegt sich um 20 Vollschritte vor und zurück."),
         t("MS1 aktivieren und die halbe Schrittweite vergleichen.")},
        {"stepper_accel", t("A4988: AccelStepper"),
         t("Dieselbe verdrahtete Schaltung mit Positionsziel und vereinfachter Beschleunigung im Sketch."),
         t("Nicht blockierende Positionssteuerung lernen."),
         t("Der Sketch steuert zwischen Position 0 und 20."),
         t("Geschwindigkeit und Beschleunigung im Sketch ändern.")},
        {"stepper_uln2003", t("ULN2003: 28BYJ-48"),
         t("Fünfleiter-Motor mit gemeinsamer 5-V-Versorgung und Stepper-Bibliothek."),
         t("Spulenfolge und Anfangsausrichtung verstehen."),
         t("Nach der Ausrichtung dreht der Motor jeweils 32 Schritte vor und zurück."),
         t("Eine Spulenleitung entfernen und die gültige Schrittfolge prüfen.")},
        {"stepper_unipolar6", t("ULN2003: Sechsleiter-Motor"),
         t("Beide Mittelanzapfungen an 5 V, vier Wicklungsenden am Treiber."),
         t("Mittelanzapfungen und unipolare Ansteuerung verstehen."),
         t("Nach der Ausrichtung bewegt sich die Welle um eine Vierteldrehung."),
         t("Schritte pro Umdrehung und Drehzahl vergleichen.")},
        {"relay_button", t("Relais: Taster und LED"),
         t("Ein Taster steuert den Relaiskontakt; die LED besitzt einen 220-Ohm-Vorwiderstand."),
         t("Steuerkreis und Lastkreis unterscheiden."),
         t("Bei gedrücktem Taster schließt COM nach NO und die LED leuchtet."),
         t("Die Last von NO nach NC umstecken und das Verhalten vergleichen.")},
        {"tft_ili9341", t("ILI9341: Farbige Grafik"),
         t("ESP32-S3 mit SPI, CS, DC, RESET und versorgter Hintergrundbeleuchtung."),
         t("RGB565, Text, Grafik und Rotation verwenden."),
         t("Text, Kreis und wechselnde farbige Flächen erscheinen auf dem TFT."),
         t("RESET oder LED-Versorgung entfernen und den Unterschied untersuchen.")},
        {"tft_st7735", t("ST7735: Kompakte Grafik"),
         t("128×160-TFT am ESP32-S3; gedreht ergibt sich eine 160×128-Ansicht."),
         t("Grafik an unterschiedliche Auflösungen anpassen."),
         t("Text und farbige Formen erscheinen auf dem kleineren Display."),
         t("Rotation zwischen Hoch- und Querformat wechseln.")},
        {"bme680_monitor", t("BME680: Umweltmonitor"),
         t("I²C-Sensor an GPIO8/9 mit originaler Adafruit-/Bosch-Bibliothek. Gaswerte sind simuliert."),
         t("Temperatur, Feuchtigkeit, Druck und Gaswiderstand auslesen."),
         t("Die Ausgabe zeigt die im Inspektor eingestellten Messwerte."),
         t("Eine andere Adresse oder einen simulierten Kommunikationsfehler ausprobieren.")},
        {"mhz19_monitor", t("MH-Z19B: CO₂ über UART"),
         t("ESP32-S3 mit 5-V-Sensorversorgung und gekreuzten 3,3-V-UART-Leitungen an GPIO17/18."),
         t("UART-Antworten und Prüfsummen prüfen."),
         t("Die Ausgabe meldet CO₂-Werte; ein falscher Prüfsummenmodus führt zum Lesefehler."),
         t("CO₂ im Inspektor verändern und den Prüfsummenfehler einschalten.")},
        {"share_roundtrip", t("Projekt teilen: Rundlauf"),
         t("Verdrahtete Blinkschaltung zum Exportieren und erneuten Importieren eines Share-Codes."),
         t("Importvorschau und Passwortschutz kennenlernen."),
         t("Die importierte Kopie enthält dieselbe Schaltung und denselben Sketch."),
         t("Mit Passwort exportieren und beim Import zunächst ein falsches Passwort testen.")}};
}
bool configureFeature135Example(const QString &id, SavedProject &project) {
    const auto entries = feature135Examples();
    if (std::none_of(entries.begin(), entries.end(), [&](const auto &e) { return e.id == id; }))
        return false;
    auto &doc = project.circuit;
    project.mode = ExecutionMode::Emulation;
    auto module = [&](const char *model, double x = 430, double y = 60) {
        auto name = doc.addPart(PartKind::Module, x, y);
        doc.part(name).model = model;
        return name;
    };
    auto connect = [&](const std::string &part, const char *terminal, const char *boardPin) {
        doc.connect({part, terminal}, {"board", boardPin});
    };
    if (id.startsWith("stepper_")) {
        doc.part("board").model = "arduino-uno";
        if (id == "stepper_a4988" || id == "stepper_accel") {
            const auto driver = module("a4988"), motor = module("stepper-4", 720, 70),
                       supply = doc.addPart(PartKind::PowerSupply, 430, 340);
            doc.part(supply).value = 12;
            connect(driver, "VDD", "5V");
            connect(driver, "GND", "GND");
            connect(driver, "GND_MOT", "GND");
            connect(driver, "STEP", "D2");
            connect(driver, "DIR", "D3");
            connect(driver, "ENABLE", "D4");
            connect(driver, "RESET", "5V");
            connect(driver, "SLEEP", "5V");
            for (const char *pin : {"MS1", "MS2", "MS3"})
                connect(driver, pin, "GND");
            doc.connect({supply, "+"}, {driver, "VMOT"});
            doc.connect({supply, "-"}, {driver, "GND_MOT"});
            if (id == "stepper_a4988") {
                const auto endstop = module("limit-switch", 420, 480);
                connect(endstop, "COM", "GND");
                connect(endstop, "NC", "D6");
            }
            for (auto [a, b] : {std::pair{"1A", "A1"}, {"1B", "A2"}, {"2A", "B1"}, {"2B", "B2"}})
                doc.connect({driver, a}, {motor, b});
        } else {
            const bool six = id == "stepper_unipolar6";
            const auto driver = module("uln2003"), motor = module(six ? "stepper-6" : "stepper-5", 760, 60);
            connect(driver, "GND", "GND");
            connect(driver, "COM", "5V");
            if (six) {
                connect(motor, "AC", "5V");
                connect(motor, "BC", "5V");
            } else
                connect(motor, "COM", "5V");
            const char *pins[] = {"A1", "B1", "A2", "B2"};
            for (int i = 0; i < 4; ++i) {
                const auto n = std::to_string(i + 1);
                doc.connect({driver, "IN" + n}, {"board", "D" + std::to_string(i + 2)});
                doc.connect({driver, "OUT" + n}, {motor, six ? std::string(pins[i]) : n});
            }
        }
    } else if (id == "relay_button") {
        doc.part("board").model = "arduino-uno";
        const auto relay = module("relay-5v"), button = doc.addPart(PartKind::Button, 330, 350);
        connect(relay, "VCC", "5V");
        connect(relay, "GND", "GND");
        connect(relay, "IN", "D2");
        connect(relay, "COM", "5V");
        connect(button, "1", "D7");
        connect(button, "2", "GND");
        const auto resistor = doc.addPart(PartKind::Resistor, 700, 80),
                   led = doc.addPart(PartKind::Led, 920, 80);
        doc.part(resistor).value = 220;
        doc.connect({relay, "NO"}, {resistor, "1"});
        doc.connect({resistor, "2"}, {led, "A"});
        connect(led, "K", "GND");
    } else {
        doc.part("board").model = "esp32-s3";
        if (id == "tft_ili9341" || id == "tft_st7735") {
            const auto panel = module(id == "tft_ili9341" ? "ili9341" : "st7735");
            connect(panel, "VCC", "3V3");
            connect(panel, "GND", "GND");
            connect(panel, "LED", "3V3");
            for (auto [terminal, pin] : {std::pair{"SCK", "GPIO12"},
                                         {"MISO", "GPIO13"},
                                         {"MOSI", "GPIO11"},
                                         {"CS", "GPIO10"},
                                         {"DC", "GPIO9"},
                                         {"RESET", "GPIO8"}})
                connect(panel, terminal, pin);
        } else if (id == "bme680_monitor") {
            const auto sensor = module("bme680");
            connect(sensor, "VCC", "3V3");
            connect(sensor, "GND", "GND");
            connect(sensor, "SDA", "GPIO8");
            connect(sensor, "SCL", "GPIO9");
        } else if (id == "mhz19_monitor") {
            const auto sensor = module("mh-z19b");
            doc.part(sensor).parameters["co2"] = 812;
            connect(sensor, "VCC", "5V");
            connect(sensor, "GND", "GND");
            connect(sensor, "TX", "GPIO17");
            connect(sensor, "RX", "GPIO18");
        } else {
            const auto resistor = doc.addPart(PartKind::Resistor, 430, 100),
                       led = doc.addPart(PartKind::Led, 650, 100);
            doc.part(resistor).value = 220;
            connect(resistor, "1", "GPIO2");
            doc.connect({resistor, "2"}, {led, "A"});
            connect(led, "K", "GND");
        }
    }
    const auto description =
        std::find_if(entries.begin(), entries.end(), [&](const auto &e) { return e.id == id; });
    const auto note = doc.addPart(PartKind::Note, 40, 620);
    doc.part(note).model =
        (description->goal + "\n" + description->expected + "\n" + description->task).toStdString();
    return true;
}
