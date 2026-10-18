#include "starterprojects.hpp"
#include "language.hpp"
#include "paths.hpp"
#include "blockeditor/blockpersistence.hpp"
#include "thinkercad/visual/expressionparser.hpp"
#include <QFile>
using namespace thinkercad;
namespace {
struct Starter {
    const char *id;
    const char *title;
    const char *base;
    const char *description;
    const char *goal;
    const char *expected;
    const char *task;
};
const Starter starters[] = {
    {"starter_blink", "Starter: LED blinken", "led_blink", "ESP8266 mit LED und 220-Ω-Vorwiderstand.",
     "Geschlossenen Stromkreis und GPIO-Ausgabe verstehen.",
     "Die LED wechselt regelmäßig zwischen an und aus.", "Blinkintervall ändern und den LED-Strom messen."},
    {"starter_button", "Starter: Taster und LED", "button_led",
     "Taster gegen GND, interner Pullup und LED mit Vorwiderstand.", "Eingänge mit Pullup lesen.",
     "Bei gedrücktem Taster leuchtet die LED.", "Einen schwebenden Eingang erzeugen und die Diagnose lesen."},
    {"starter_pwm", "Starter: PWM-Dimmung", "", "Uno D5 steuert eine LED über 220 Ω.",
     "PWM-Tastverhältnis und Helligkeit vergleichen.", "Die LED wird heller und dunkler.",
     "PWM und gefilterte Spannung im Oszilloskop vergleichen."},
    {"starter_servo", "Starter: Servo", "servo_sweep", "Servo an Uno D9 mit 5-V-Versorgung.",
     "Servopulse und Winkelsteuerung kennenlernen.", "Der Servo bewegt sich zwischen 15 und 165 Grad.",
     "Bewegungsbereich und Geschwindigkeit ändern."},
    {"starter_pot", "Starter: Potentiometer", "",
     "10-kΩ-Potentiometer zwischen 5 V und GND, Schleifer an A0.",
     "Analoge Spannung messen und in PWM umsetzen.",
     "Potentiometerstellung verändert ADC-Wert und LED-Helligkeit.",
     "ADC-Wert bei 25, 50 und 75 Prozent vergleichen."},
    {"starter_temperature", "Starter: OLED-Temperatur", "",
     "ESP32 mit TMP36 an GPIO34 und SSD1306 an GPIO21/22.",
     "Analoge Sensordaten auf einem I²C-Display ausgeben.",
     "Das OLED zeigt ungefähr 25 °C; Sensorwert lässt sich ändern.",
     "Temperatur ändern und die Anzeige beobachten."},
    {"starter_dht", "Starter: DHT22", "", "DHT22 mit 5 V, GND und DATA an Uno D2.",
     "Temperatur, Feuchtigkeit und Sensorintervall verstehen.",
     "Temperatur und Feuchtigkeit erscheinen alle 2,1 Sekunden seriell.",
     "Feuchtigkeit ändern und die nächste Messung abwarten."},
    {"starter_ultrasonic", "Starter: Ultraschall", "ultrasonic_distance",
     "HC-SR04 an Uno D2/TRIG und D3/ECHO, mit 5 V/GND.", "Laufzeitmessung in Abstand umrechnen.",
     "Der serielle Monitor zeigt den eingestellten Abstand.",
     "Abstand ändern und die Echo-Pulsdauer messen."},
    {"starter_pixels", "Starter: WS2812B", "neopixel_strip",
     "Acht adressierbare RGB-Pixel an Uno D6 mit 5 V/GND.",
     "Digitale Farbwerte und modellierte Übertragungszeiten verstehen.",
     "Die acht Pixel zeigen eine laufende Farbanimation.",
     "Pixelfarben ändern; Transaktionsmodell und GPIO-Spur unterscheiden."},
    {"starter_motor", "Starter: Motor mit L298N", "",
     "Uno steuert einen L298N mit separater 9-V-Motorversorgung und gemeinsamem GND.",
     "Logikversorgung, Motorversorgung und H-Brücke unterscheiden.",
     "Der Motor läuft vorwärts mit PWM; Anlaufstrom und Drehzahl verändern sich.",
     "Richtung durch Vertauschen der IN1-/IN2-Pegel ändern."},
    {"starter_scanner", "Starter: I²C-Scanner", "", "ESP32 mit SSD1306 an 0x3C und BME280 an 0x76.",
     "Erreichbare Geräteadressen am gemeinsamen I²C-Bus suchen.", "Der Scanner findet 0x3C und 0x76.",
     "Ein Gerät abtrennen oder einen Adresskonflikt erzeugen."},
    {"starter_uart", "Starter: UART zwischen Boards", "uart_pair",
     "Zwei ESP32 mit gekreuzten GPIO16/17 und gemeinsamem GND.",
     "Unabhängige Sketches und serielle Kommunikation verstehen.",
     "PING/PONG-Austausch erzeugt die Ausgabe UART_OK.",
     "Eine Leitung vertauschen und die UART-Diagnose prüfen."}};
} // namespace
QList<CircuitExample> StarterProjects::entries() {
    QList<CircuitExample> result;
    for (const auto &s : starters)
        result.append({s.id, Language::text(s.title), Language::text(s.description), Language::text(s.goal),
                       Language::text(s.expected), Language::text(s.task)});
    return result;
}
QString StarterProjects::instructions(const QString &id) {
    for (const auto &s : starters)
        if (id == s.id) {
            QString html;
            for (const auto &pair :
                 {std::pair{"Beschreibung", s.description}, std::pair{"Lernziel", s.goal},
                  std::pair{"Erwartetes Verhalten", s.expected}, std::pair{"Aufgabe", s.task}})
                html += "<h3>" + Language::text(pair.first).toHtmlEscaped() + "</h3><p>" +
                        Language::text(pair.second).toHtmlEscaped() + "</p>";
            return html;
        }
    return {};
}
SavedProject StarterProjects::create(const QString &id) {
    const Starter *starter = nullptr;
    for (const auto &s : starters)
        if (id == s.id)
            starter = &s;
    if (!starter)
        throw std::runtime_error("Unknown starter project");
    if (*starter->base) {
        auto project = ExampleCatalog::create(starter->base);
        if (id == "starter_blink" || id == "starter_button") {
            using namespace thinkercad::visual;
            auto expression = [](const char *s) { return ExpressionParser::parse(s); };
            auto command = [&](const char *id, const char *name, std::vector<Expression> args) {
                Block b;
                b.id = id;
                b.name = name;
                b.arguments = std::move(args);
                return b;
            };
            BlockProgram p;
            p.setup = {command("led-mode", "pinMode", {expression("14"), expression("OUTPUT")})};
            if (id == "starter_button") {
                p.setup.push_back(
                    command("button-mode", "pinMode", {expression("12"), expression("INPUT_PULLUP")}));
                Block condition;
                condition.id = "button-if";
                condition.kind = Block::Kind::If;
                condition.arguments = {expression("digitalRead(12) == LOW")};
                condition.body = {command("led-on", "digitalWrite", {expression("14"), expression("HIGH")})};
                condition.otherwise = {
                    command("led-off", "digitalWrite", {expression("14"), expression("LOW")})};
                p.loop = {condition, command("button-delay", "delay", {expression("10")})};
            } else
                p.loop = {command("led-on", "digitalWrite", {expression("14"), expression("HIGH")}),
                          command("on-delay", "delay", {expression("500")}),
                          command("led-off", "digitalWrite", {expression("14"), expression("LOW")}),
                          command("off-delay", "delay", {expression("500")})};
            const auto generated =
                ArduinoCodeGenerator::generate(p, BoardProfile::get(project.circuit.part("board").model));
            if (!generated.valid())
                throw std::runtime_error("Invalid visual starter");
            const auto source = QString::fromStdString(generated.source);
            project.sketch.files[project.sketch.entryPoint()] = source;
            project.sketch.visual = {{"program", BlockPersistence::encode(p)},
                                     {"generator", int(ArduinoCodeGenerator::version)},
                                     {"fingerprint", BlockPersistence::fingerprint(source)},
                                     {"mode", 1}};
        }
        return project;
    }
    SavedProject project;
    auto &doc = project.circuit;
    doc.part("board").model = "arduino-uno";
    project.mode = ExecutionMode::Emulation;
    auto module = [&](const char *model, double x = 500, double y = 80) {
        auto part = doc.addPart(PartKind::Module, x, y);
        doc.part(part).model = model;
        return part;
    };
    auto power = [&](const std::string &part, const char *rail) {
        doc.connect({part, "VCC"}, {"board", rail});
        doc.connect({part, "GND"}, {"board", "GND"});
    };
    auto led = [&] {
        auto r = doc.addPart(PartKind::Resistor, 400, 300), l = doc.addPart(PartKind::Led, 650, 300);
        doc.connect({"board", "D5"}, {r, "1"});
        doc.connect({r, "2"}, {l, "A"});
        doc.connect({l, "K"}, {"board", "GND"});
    };
    if (id == "starter_pwm" || id == "starter_pot") {
        led();
        if (id == "starter_pot") {
            auto pot = doc.addPart(PartKind::Potentiometer, 500, 80);
            doc.connect({pot, "1"}, {"board", "GND"});
            doc.connect({pot, "2"}, {"board", "5V"});
            doc.connect({pot, "W"}, {"board", "A0"});
        }
    } else if (id == "starter_dht") {
        auto d = module("dht22");
        power(d, "5V");
        doc.connect({d, "DATA"}, {"board", "D2"});
    } else if (id == "starter_motor") {
        auto bridge = module("l298n"), motor = doc.addPart(PartKind::DcMotor, 800, 80),
             supply = doc.addPart(PartKind::Battery9V, 500, 430);
        power(bridge, "5V");
        doc.connect({supply, "-"}, {"board", "GND"});
        doc.connect({supply, "+"}, {bridge, "VS"});
        doc.connect({bridge, "OUT1"}, {motor, "+"});
        doc.connect({bridge, "OUT2"}, {motor, "-"});
        for (const auto &[pin, gpio] : std::vector<std::pair<std::string, int>>{
                 {"ENA", 5}, {"IN1", 4}, {"IN2", 7}, {"ENB", 6}, {"IN3", 8}, {"IN4", 9}})
            doc.connect({bridge, pin}, {"board", "D" + std::to_string(gpio)});
    } else {
        doc.part("board").model = "esp32";
        auto oled = doc.addPart(PartKind::Oled, 600, 80);
        power(oled, "3V3");
        doc.connect({oled, "SDA"}, {"board", "GPIO21"});
        doc.connect({oled, "SCL"}, {"board", "GPIO22"});
        if (id == "starter_temperature") {
            auto sensor = doc.addPart(PartKind::TemperatureSensor, 400, 400);
            doc.part(sensor).value = 25;
            power(sensor, "3V3");
            doc.connect({sensor, "OUT"}, {"board", "GPIO34"});
        } else {
            auto sensor = module("bme280", 900, 80);
            power(sensor, "3V3");
            doc.connect({sensor, "SDA"}, {"board", "GPIO21"});
            doc.connect({sensor, "SCL"}, {"board", "GPIO22"});
        }
    }
    QFile file(AppPaths::examples() + "/" + id + ".ino");
    if (!file.open(QIODevice::ReadOnly))
        throw std::runtime_error("Starter sketch missing");
    project.sketch.name = id;
    project.sketch.order = {id + ".ino"};
    project.sketch.files[id + ".ino"] = QString::fromUtf8(file.readAll());
    return project;
}
