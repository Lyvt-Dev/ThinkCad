#include "thinkercad/actuators.hpp"
#include "thinkercad/diagnostics.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace thinkercad;
static void check(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}
int main() {
    try {
        for (const auto &[id, model] : ModuleRegistry::definitions())
            ModuleRegistry::validate(model);
        CircuitDocument doc;
        doc.part("board").model = "arduino-uno";
        Esp8266Board board;
        board.configure("arduino-uno");
        auto add = [&](const char *model) {
            auto id = doc.addPart(PartKind::Module, 0, 0);
            doc.part(id).model = model;
            return id;
        };
        const auto driver = add("a4988"), motor = add("stepper-4");
        auto power = doc.addPart(PartKind::PowerSupply, 0, 0);
        doc.part(power).value = 12;
        doc.connect({power, "+"}, {driver, "VMOT"});
        doc.connect({power, "-"}, {driver, "GND_MOT"});
        doc.connect({driver, "GND"}, {"board", "GND"});
        doc.connect({driver, "VDD"}, {"board", "5V"});
        doc.connect({driver, "RESET"}, {"board", "5V"});
        doc.connect({driver, "SLEEP"}, {"board", "5V"});
        doc.connect({driver, "STEP"}, {"board", "D2"});
        doc.connect({driver, "DIR"}, {"board", "D3"});
        doc.connect({driver, "ENABLE"}, {"board", "D4"});
        for (auto [a, b] : {std::pair{"1A", "A1"}, {"1B", "A2"}, {"2A", "B1"}, {"2B", "B2"}})
            doc.connect({driver, a}, {motor, b});
        for (int pin : {2, 3, 4})
            board.pinMode(pin, PinMode::Output);
        board.digitalWrite(3, true);
        TransientSimulation sim;
        auto run = [&] {
            auto s = sim.evaluate(doc, {{"board", &board}});
            check(s.errors.empty(), "Actor circuit must converge");
            return s;
        };
        run();
        auto pulse = [&] {
            board.digitalWrite(2, false);
            run();
            board.digitalWrite(2, true);
            return run();
        };
        CircuitState state;
        for (int i = 0; i < 20; ++i)
            state = pulse();
        check(std::abs(state.partValues.at(driver) - 20) < 1e-6, "STEP counts rising edges once");
        check(std::abs(state.partValues.at(motor) - 36) < 1e-6, "200-step motor shaft angle");
        check(state.partValues.at(motor + "/coils") > 0, "Enabled A4988 exposes coil state");
        auto healthy = DiagnosticEngine::analyze(doc, NetTopology(doc), state, {{"board", &board}});
        check(std::none_of(healthy.begin(), healthy.end(),
                           [](const auto &d) { return d.rule == "stepper-supply"; }),
              "Powered driver must not receive missing-supply warning");
        doc.part(power).value = 0;
        auto unpowered = run();
        auto problems = DiagnosticEngine::analyze(doc, NetTopology(doc), unpowered, {{"board", &board}});
        check(std::any_of(problems.begin(), problems.end(),
                          [&](const auto &d) {
                              return d.rule == "stepper-supply" && d.target.part == driver &&
                                     !d.solution.empty();
                          }),
              "Unpowered connected driver has actionable diagnosis");
        doc.part(power).value = 12;
        run();
        for (int i = 0; i < 4; ++i)
            run();
        check(run().partValues.at(driver) == 20, "Repeated solver passes must not create steps");
        board.digitalWrite(3, false);
        state = pulse();
        check(state.partValues.at(driver) == 19, "DIR reverses direction");
        doc.connect({driver, "MS1"}, {"board", "5V"});
        state = pulse();
        check(state.partValues.at(driver) == 18.5, "Half-step mode");
        doc.connect({driver, "MS2"}, {"board", "5V"});
        doc.connect({driver, "MS3"}, {"board", "5V"});
        state = pulse();
        check(state.partValues.at(driver) == 18.4375, "Sixteenth-step mode");
        board.digitalWrite(4, true);
        state = pulse();
        check(state.partValues.at(driver) == 18.4375, "ENABLE high disables stepping");
        check(state.partValues.at(motor + "/coils") == 0 && state.partValues.at(motor + "/rpm") == 0,
              "Disabled motor has no energized coils or inferred RPM");
        board.digitalWrite(4, false);
        doc.part(power).value = 0;
        state = pulse();
        check(state.partValues.at(driver) == 18.4375, "Missing motor supply must prevent motion");
        const auto relay = add("relay-5v");
        doc.connect({relay, "VCC"}, {"board", "5V"});
        doc.connect({relay, "GND"}, {"board", "GND"});
        doc.connect({relay, "IN"}, {"board", "D5"});
        board.pinMode(5, PinMode::Output);
        board.digitalWrite(5, false);
        doc.connect({relay, "COM"}, {"board", "3V3"});
        for (const char *contact : {"NO", "NC"}) {
            auto r = doc.addPart(PartKind::Resistor, 0, 0);
            doc.part(r).value = 10000;
            doc.connect({relay, contact}, {r, "1"});
            doc.connect({r, "2"}, {"board", "GND"});
        }
        state = run();
        check(state.partValues.at(relay) == 0, "Relay idle state");
        check(state.voltage.at({relay, "NC"}).value_or(0) > 3,
              "Normally closed contact conducts without coil");
        check(state.voltage.at({relay, "NO"}).value_or(1) < .1, "Normally open contact is isolated");
        board.digitalWrite(5, true);
        state = run();
        check(state.partValues.at(relay) == 1, "Powered input energizes relay");
        check(state.voltage.at({relay, "NO"}).value_or(0) > 3, "Energized relay connects NO");
        doc.part(relay).parameters["activeLow"] = 1;
        state = run();
        check(state.partValues.at(relay) == 0, "Active-low relay option");
        board.digitalWrite(5, false);
        state = run();
        check(state.partValues.at(relay) == 1, "Active-low relay energizes on LOW");
        for (const char *model : {"stepper-5", "stepper-6"}) {
            CircuitDocument unipolar;
            unipolar.part("board").model = "arduino-uno";
            auto d = unipolar.addPart(PartKind::Module, 0, 0);
            unipolar.part(d).model = "uln2003";
            auto m = unipolar.addPart(PartKind::Module, 0, 0);
            unipolar.part(m).model = model;
            unipolar.connect({d, "GND"}, {"board", "GND"});
            unipolar.connect({d, "COM"}, {"board", "5V"});
            if (std::string(model) == "stepper-5")
                unipolar.connect({m, "COM"}, {"board", "5V"});
            else {
                unipolar.connect({m, "AC"}, {"board", "5V"});
                unipolar.connect({m, "BC"}, {"board", "5V"});
            }
            const char *six[] = {"A1", "B1", "A2", "B2"};
            for (int i = 0; i < 4; ++i) {
                auto n = std::to_string(i + 1);
                unipolar.connect({d, "IN" + n}, {"board", "D" + std::to_string(i + 2)});
                unipolar.connect({d, "OUT" + n}, {m, std::string(model) == "stepper-5" ? n : six[i]});
                board.pinMode(i + 2, PinMode::Output);
                board.digitalWrite(i + 2, false);
            }
            TransientSimulation engine;
            for (int mask : {1, 3, 2, 6, 4, 12, 8, 9, 1}) {
                for (int i = 0; i < 4; ++i)
                    board.digitalWrite(i + 2, (mask >> i) & 1);
                state = engine.evaluate(unipolar, {{"board", &board}});
                check(state.errors.empty(), "ULN2003 circuit converges");
            }
            check(state.partValues.at(m + "/steps") == 4,
                  "ULN2003 half-step sequence rotates 5/6-wire motor");
            // Real Stepper sketches write each GPIO separately, in constructor pin order.
            for (int i = 0; i < 4; ++i)
                board.digitalWrite(i + 2, (3 >> i) & 1);
            engine.reset();
            state = engine.evaluate(unipolar, {{"board", &board}});
            for (int mask : {6, 12, 9, 3})
                for (int i : {0, 2, 1, 3}) {
                    board.digitalWrite(i + 2, (mask >> i) & 1);
                    state = engine.evaluate(unipolar, {{"board", &board}});
                }
            check(state.partValues.at(m + "/steps") == 4,
                  "Sequential GPIO writes must not lose full steps at invalid intermediate coil masks");
            for (int i = 0; i < 4; ++i)
                board.digitalWrite(i + 2, (3 >> i) & 1);
            engine.reset();
            engine.evaluate(unipolar, {{"board", &board}});
            for (int mask : {6, 12, 9, 3}) {
                engine.advance(unipolar, {{"board", &board}}, engine.now() + 10000);
                for (int i : {0, 2, 1, 3}) {
                    board.digitalWrite(i + 2, (mask >> i) & 1);
                    state = engine.evaluate(unipolar, {{"board", &board}});
                }
                check(std::abs(state.partValues.at(m + "/stepRate") - 100) < 1e-6,
                      "Sequential coil writes at one timestamp must retain commanded step rate");
                check(std::abs(state.partValues.at(m + "/rpm") -
                               6000 / parameter(unipolar.part(m), "steps",
                                                std::string(model) == "stepper-5" ? 2048 : 200)) < 1e-6,
                      "RPM follows full steps and configured motor resolution");
            }
            for (int i = 0; i < 4; ++i)
                board.digitalWrite(i + 2, false);
            state = engine.evaluate(unipolar, {{"board", &board}});
            check(state.partValues.at(m + "/rpm") == 0 && state.partValues.at(m + "/coils") == 0,
                  "Unenergized windings report zero RPM and no active coils");
        }
        CircuitDocument endstop;
        endstop.part("board").model = "arduino-uno";
        auto sw = endstop.addPart(PartKind::Module, 0, 0);
        endstop.part(sw).model = "limit-switch";
        endstop.connect({sw, "COM"}, {"board", "GND"});
        endstop.connect({sw, "NC"}, {"board", "D6"});
        board.pinMode(6, PinMode::InputPullup);
        check(endstop.solve(board).level({"board", "D6"}) == Level::Low, "Closed NC endstop connects to GND");
        endstop.part(sw).parameters["pressed"] = 1;
        check(endstop.solve(board).level({"board", "D6"}) == Level::High,
              "Actuated NC endstop opens and input pulls up");
        std::cout << "Actuator wiring, direction, microstepping and relay contacts passed\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
