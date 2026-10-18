#include "thinkercad/visual/blockprogram.hpp"
#include "thinkercad/visual/expressionparser.hpp"
#include "thinkercad/diagnostics.hpp"
#include <iostream>
#include <stdexcept>
using namespace thinkercad;
using namespace thinkercad::visual;
void check(bool ok, const char *message) {
    if (!ok)
        throw std::runtime_error(message);
}
Expression e(const char *s) {
    return ExpressionParser::parse(s);
}
Block call(std::string id, std::string name, std::vector<Expression> args) {
    Block b;
    b.id = id;
    b.name = name;
    b.arguments = args;
    return b;
}
int main() {
    try {
        BlockProgram p;
        p.setup = {call("mode", "pinMode", {e("5"), e("OUTPUT")})};
        p.loop = {call("high", "digitalWrite", {e("5"), e("HIGH")}), call("wait", "delay", {e("100")})};
        auto profile = BoardProfile::get("esp8266");
        auto a = ArduinoCodeGenerator::generate(p, profile);
        check(a.valid(), "blink validates");
        check(a.source == ArduinoCodeGenerator::generate(p, profile).source, "deterministic generation");
        check(a.source.find("digitalWrite(5, HIGH);") != std::string::npos, "readable code");
        check(!a.lineBlocks.empty(), "source map");
        p.loop[0].arguments[0] = e("99");
        check(!ArduinoCodeGenerator::generate(p, profile).valid(), "board pins validated");
        p.loop[0].arguments[0] = e("5");
        p.loop[0].name = "system";
        check(!ArduinoCodeGenerator::generate(p, profile).valid(), "arbitrary calls rejected");
        p.loop[0].name = "digitalWrite";
        p.loop[1].id = "high";
        check(!ArduinoCodeGenerator::generate(p, profile).valid(), "duplicate IDs rejected");
        for (const auto *source : {"0); system(1)", "#include <x>", "foo;bar", "x = 2"}) {
            bool rejected = false;
            try {
                e(source);
            } catch (...) {
                rejected = true;
            }
            check(rejected, "statement injection rejected");
        }
        BlockProgram advanced;
        advanced.globals = {{"count", ValueType::Integer, e("08")}};
        advanced.setup = {call("servo", "servo.attach", {e("9")}), call("wire", "Wire.begin", {}),
                          call("display", "display.begin", {e("SSD1306_SWITCHCAPVCC"), e("0x3C")})};
        Block condition;
        condition.id = "if";
        condition.kind = Block::Kind::If;
        condition.arguments = {e("count + 2 * 3 > 4 && true")};
        condition.body = {call("serial", "Serial.println", {e("\"Hello\\nworld\"")})};
        condition.otherwise = {call("servo-write", "servo.write", {e("90")})};
        advanced.loop = {condition};
        auto result = ArduinoCodeGenerator::generate(advanced, BoardProfile::get("arduino-uno"));
        check(result.valid(), "structured code validates");
        check(result.source.find("int32_t count = 8;") != std::string::npos, "decimal numbers canonicalized");
        check(result.source.find("#include <Servo.h>") != std::string::npos, "required library included");
        check(result.source.find("display.begin(SSD1306_SWITCHCAPVCC, 60)") != std::string::npos,
              "hex literal parsed");
        CircuitDocument doc;
        auto bb = doc.addPart(PartKind::Breadboard30, 0, 0);
        doc.connect({"board", "GPIO5"}, {bb, "a1"});
        NetTopology net(doc);
        check(net.connected({bb, "a1"}, {bb, "e1"}), "five contacts connected");
        check(!net.connected({bb, "a1"}, {bb, "f1"}), "trench isolates halves");
        check(!net.connected({bb, "a1"}, {bb, "a2"}), "columns isolated");
        check(net.members({bb, "e1"}).size() == 6, "board pin included");
        auto old = doc.wires().size();
        check(net.preview({bb, "a1"}, {bb, "f1"}).size() == 11 && doc.wires().size() == old,
              "preview never mutates");
        auto signature = NetTopology::signature(doc);
        doc.part(bb).x += 100;
        check(signature == NetTopology::signature(doc), "geometry does not invalidate topology");
        doc.part(bb).model = "power-rails";
        NetTopology rails(doc);
        check(rails.connected({bb, "T+1"}, {bb, "T+25"}), "power rail connected");
        check(!rails.connected({bb, "T+1"}, {bb, "T-1"}), "opposite rails isolated");
        Esp8266Board board;
        std::map<std::string, const Esp8266Board *> boards{{"board", &board}};
        doc.connect({"board", "3V3"}, {"board", "GND"});
        auto issues = DiagnosticEngine::analyze(doc, NetTopology(doc), {}, boards);
        check(std::any_of(issues.begin(), issues.end(), [](auto &i) { return i.rule == "driver-conflict"; }),
              "supply short explained");
        doc.clearWires();
        auto led = doc.addPart(PartKind::Led, 0, 0);
        doc.connect({"board", "3V3"}, {led, "A"});
        doc.connect({"board", "GND"}, {led, "K"});
        auto bad = DiagnosticEngine::analyze(doc, NetTopology(doc), doc.solve(board), boards);
        check(std::any_of(bad.begin(), bad.end(), [](auto &i) { return i.rule == "led-overcurrent"; }),
              "LED current diagnosed");
        {
            CircuitDocument d;
            auto input = d.addPart(PartKind::Breadboard30, 0, 0);
            d.connect({"board", "GPIO4"}, {input, "a1"});
            Esp8266Board b;
            std::map<std::string, const Esp8266Board *> bs{{"board", &b}};
            auto has = [&](const std::string &rule) {
                auto list = DiagnosticEngine::analyze(d, NetTopology(d), d.solve(bs), bs);
                return std::any_of(list.begin(), list.end(), [&](const auto &i) { return i.rule == rule; });
            };
            check(has("floating-input"), "wired floating input diagnosed");
            b.pinMode(4, PinMode::InputPullup);
            check(!has("floating-input"), "pullup is not a floating input");
            auto sensor = d.addPart(PartKind::Module, 0, 0);
            d.part(sensor).model = "bme280";
            check(has("supply"), "missing device supply diagnosed");
            d.connect({sensor, "VCC"}, {"board", "3V3"});
            d.connect({sensor, "GND"}, {"board", "GND"});
            check(!has("supply"), "valid device supply accepted");
            auto other = d.addPart(PartKind::Module, 0, 0);
            d.part(other).model = "bme280";
            for (auto id : {sensor, other}) {
                d.connect({id, "SDA"}, {"board", "GPIO4"});
                d.connect({id, "SCL"}, {"board", "GPIO5"});
            }
            check(has("i2c-address-conflict"), "same address on same bus diagnosed");
            d.part(other).parameters["address"] = 0x77;
            check(!has("i2c-address-conflict"), "distinct I2C addresses accepted");
            BusTraffic traffic;
            traffic.enabled = true;
            traffic.append({0, "board", "I2C", "0x76", "ACK", 400000, {0xd0}, {0x60}});
            auto list = DiagnosticEngine::analyze(d, NetTopology(d), d.solve(bs), bs, nullptr, &traffic);
            check(
                std::none_of(list.begin(), list.end(), [](const auto &i) { return i.rule == "i2c-access"; }),
                "acknowledged I2C not falsely diagnosed");
            traffic.append({1, "board", "I2C", "0x75", "NACK 2", 400000, {0xd0}, {}});
            list = DiagnosticEngine::analyze(d, NetTopology(d), d.solve(bs), bs, nullptr, &traffic);
            check(std::any_of(list.begin(), list.end(), [](const auto &i) { return i.rule == "i2c-access"; }),
                  "failed real I2C access explained");
        }
        {
            CircuitDocument d;
            d.part("board").model = "esp32";
            auto second = d.addPart(PartKind::Esp8266, 0, 0);
            d.part(second).model = "esp32";
            Esp8266Board a, b;
            a.configure("esp32");
            b.configure("esp32");
            std::map<std::string, const Esp8266Board *> bs{{"board", &a}, {second, &b}};
            UartBus uart;
            UartBus::Config config{9600, 16, 17};
            uart.begin(d, "board", 1, config);
            uart.begin(d, second, 1, config);
            d.connect({"board", "GPIO17"}, {second, "GPIO17"});
            d.connect({"board", "GPIO16"}, {second, "GPIO16"});
            auto list = DiagnosticEngine::analyze(d, NetTopology(d), d.solve(bs), bs, &uart);
            check(std::any_of(list.begin(), list.end(), [](const auto &i) { return i.rule == "uart-tx"; }),
                  "TX/TX identified from real UART configuration");
            check(std::any_of(list.begin(), list.end(), [](const auto &i) { return i.rule == "uart-rx"; }),
                  "RX/RX identified");
            d.clearWires();
            d.connect({"board", "GPIO17"}, {second, "GPIO16"});
            d.connect({"board", "GPIO16"}, {second, "GPIO17"});
            list = DiagnosticEngine::analyze(d, NetTopology(d), d.solve(bs), bs, &uart);
            check(std::none_of(list.begin(), list.end(),
                               [](const auto &i) { return i.rule.starts_with("uart-"); }),
                  "crossed UART wiring accepted");
        }
        {
            CircuitDocument d;
            d.removePart("board");
            auto battery = d.addPart(PartKind::Battery9V, 0, 0), r = d.addPart(PartKind::Resistor, 0, 0),
                 led = d.addPart(PartKind::Led, 0, 0);
            d.connect({battery, "+"}, {r, "1"});
            d.connect({r, "2"}, {led, "A"});
            d.connect({led, "K"}, {battery, "-"});
            auto list = DiagnosticEngine::analyze(d, NetTopology(d),
                                                  d.solve(std::map<std::string, const Esp8266Board *>{}), {});
            check(
                std::none_of(list.begin(), list.end(), [](const auto &i) { return i.rule == "led-return"; }),
                "floating battery return path does not require board GND");
        }
        {
            BlockProgram p;
            Block loop;
            loop.id = "counting";
            loop.kind = Block::Kind::For;
            loop.name = "i";
            loop.arguments = {e("3"), e("0"), e("-1")};
            loop.body = {call("print-index", "Serial.println", {e("i")})};
            p.loop = {loop};
            check(ArduinoCodeGenerator::generate(p, profile).valid(),
                  "descending for loop with scoped index");
            p.loop[0].arguments[2] = e("0");
            check(!ArduinoCodeGenerator::generate(p, profile).valid(), "zero loop step rejected");
            Function f;
            f.id = "typed-function";
            f.name = "answer";
            f.result = ValueType::Integer;
            p.functions = {f};
            check(!ArduinoCodeGenerator::generate(p, profile).valid(), "non-void function must return");
        }
        std::cout << "Visual generation, expression safety, breadboard topology and diagnostics passed\n";
    } catch (const std::exception &ex) {
        std::cerr << ex.what() << '\n';
        return 1;
    }
}
