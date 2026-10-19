#include "thinkercad/actuators.hpp"
#include "thinkercad/nettopology.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <numbers>

namespace thinkercad {
namespace {
using Registers = std::array<std::uint64_t, 16>;
bool motor(const std::string &model) {
    return model == "stepper-4" || model == "stepper-5" || model == "stepper-6";
}
std::int64_t position(const Registers &r) {
    return std::bit_cast<std::int64_t>(r[0]);
}
void move(Registers &r, int delta) {
    r[0] += static_cast<std::uint64_t>(delta);
}
} // namespace
void registerActuators(std::map<std::string, ModuleDefinition> &models) {
    auto add = [&](const char *id, const char *name, std::vector<std::string> pins,
                   std::vector<ModuleParameter> parameters, double width, double height,
                   const char *power = "VCC", double low = 4.5, double high = 5.5) {
        ModuleDefinition d;
        d.id = id;
        d.label = name;
        d.implementation = id;
        d.bus = "electrical";
        d.pins = std::move(pins);
        d.parameters = std::move(parameters);
        d.width = width;
        d.height = height;
        d.power = power;
        d.ground = d.power.empty() ? "" : "GND";
        d.minVoltage = std::max(.001, low);
        d.maxVoltage = std::min(48., high);
        d.idleCurrent = d.power.empty() ? 0 : .002;
        d.builtin = true;
        const auto half = (d.pins.size() + 1) / 2;
        for (std::size_t i = 0; i < d.pins.size(); ++i)
            d.pinPositions[d.pins[i]] = {i < half ? 0 : width,
                                         2 + (i < half ? i : d.pins.size() - 1 - i) * 2.54};
        models.emplace(d.id, std::move(d));
    };
    add("a4988", "A4988 · STEP/DIR stepper driver",
        {"ENABLE", "MS1", "MS2", "MS3", "RESET", "SLEEP", "STEP", "DIR", "GND", "VDD", "1B", "1A", "2A", "2B",
         "GND_MOT", "VMOT"},
        {}, 15.2, 20.3, "VDD", 3, 5.5);
    add("uln2003", "ULN2003 · unipolar stepper driver",
        {"IN1", "IN2", "IN3", "IN4", "GND", "COM", "OUT4", "OUT3", "OUT2", "OUT1"}, {}, 32, 35, "", 0, 50);
    for (int wires : {4, 5, 6}) {
        auto pins = wires == 4   ? std::vector<std::string>{"A1", "A2", "B1", "B2"}
                    : wires == 5 ? std::vector<std::string>{"1", "2", "3", "4", "COM"}
                                 : std::vector<std::string>{"A1", "AC", "A2", "B1", "BC", "B2"};
        const auto id = "stepper-" + std::to_string(wires);
        add(id.c_str(),
            wires == 5   ? "28BYJ-48 · 5-wire stepper"
            : wires == 4 ? "Bipolar stepper · 4 wire"
                         : "Unipolar stepper · 6 wire",
            pins,
            {{"steps", "Full steps per revolution", "", wires == 5 ? 2048. : 200., 1, 100000},
             {"resistance", "Winding resistance", "Ω", wires == 5 ? 50. : 10., 1, 1000}},
            wires == 5 ? 28 : 42, wires == 5 ? 28 : 42, "", 0, 50);
    }
    add("limit-switch", "Lever endstop · SPDT", {"COM", "NO", "NC"},
        {{"pressed", "Endstop actuated", "", 0, 0, 1}}, 20, 10, "", .001, 48);
    add("relay-5v", "Relay module · 5 V SPDT", {"VCC", "GND", "IN", "COM", "NO", "NC"},
        {{"activeLow", "Active-low trigger", "", 0, 0, 1}}, 26, 34);
}
bool actuatorBranches(const CircuitPart &p,
                      const std::function<std::optional<double>(const std::string &)> &v,
                      const DeviceRegisters *memory, std::vector<ElectricalBranch> &out) {
    auto branch = [&](const char *a, const char *b, double r, double offset = 0) {
        out.push_back({a, b, r, offset});
    };
    if (p.model == "limit-switch") {
        out.push_back({"COM", parameter(p, "pressed", 0) > 0 ? "NO" : "NC", .05, 0});
        return true;
    }
    if (motor(p.model)) {
        const auto r = parameter(p, "resistance", p.model == "stepper-5" ? 50 : 10);
        if (p.model == "stepper-5")
            for (const char *pin : {"1", "2", "3", "4"})
                branch(pin, "COM", r);
        else if (p.model == "stepper-6")
            for (auto [a, b] : {std::pair{"A1", "AC"}, {"A2", "AC"}, {"B1", "BC"}, {"B2", "BC"}})
                branch(a, b, r);
        else {
            branch("A1", "A2", r);
            branch("B1", "B2", r);
        }
        return true;
    }
    auto value = [&](const char *pin) { return v(pin).value_or(0); };
    if (p.model == "uln2003") {
        for (int i = 1; i <= 4; ++i) {
            const auto in = "IN" + std::to_string(i), output = "OUT" + std::to_string(i);
            out.push_back({in, "GND", 2700, 0});
            if (v("GND") && v(in) && *v(in) - value("GND") >= 2.4)
                out.push_back({output, "GND", 5, .9});
        }
        return true;
    }
    if (p.model == "relay-5v") {
        const auto supply = v("VCC"), ground = v("GND"), input = v("IN");
        bool on = false;
        if (supply && ground && input && *supply - *ground >= 4.5 && *supply - *ground <= 5.5) {
            const double relative = *input - *ground;
            on = parameter(p, "activeLow", 0) ? relative >= -.3 && relative <= .8
                                              : relative >= 2.4 && relative <= *supply - *ground + .3;
        }
        branch("IN", "GND", 10000);
        branch("VCC", "GND", on ? 70 : 10000);
        branch("COM", on ? "NO" : "NC", .05);
        return true;
    }
    if (p.model != "a4988")
        return false;
    branch("GND", "GND_MOT", .01);
    branch("VDD", "GND", 5000);
    branch("VMOT", "GND_MOT", 10000);
    branch("ENABLE", "GND", 100000);
    branch("MS1", "GND", 100000);
    branch("MS2", "GND", 50000);
    branch("MS3", "GND", 100000);
    const double logic = value("VDD") - value("GND"), vmot = value("VMOT") - value("GND_MOT");
    auto high = [&](const char *pin) { return v(pin) && *v(pin) - value("GND") >= logic * .7; };
    if (!v("VDD") || !v("GND") || !v("VMOT") || !v("GND_MOT") || logic < 3 || logic > 5.5 || vmot < 8 ||
        vmot > 35 || high("ENABLE") || !high("RESET") || !high("SLEEP") || !memory || !memory->contains(p.id))
        return true;
    const double phase = position(memory->at(p.id)) * std::numbers::pi / 32 + std::numbers::pi / 4;
    // Quasistatic bipolar winding drive. Chopper current, back EMF and thermal protection are not modeled.
    const double a = std::cos(phase), b = std::sin(phase);
    branch("1A", "GND_MOT", 5, vmot * std::max(0., a));
    branch("1B", "GND_MOT", 5, vmot * std::max(0., -a));
    branch("2A", "GND_MOT", 5, vmot * std::max(0., b));
    branch("2B", "GND_MOT", 5, vmot * std::max(0., -b));
    return true;
}
void observeActuators(const CircuitDocument &doc, const CircuitState &state, DeviceRegisters &memory,
                      Microseconds now) {
    std::optional<NetTopology> topology;
    for (const auto &part : doc.parts())
        if (part.kind == PartKind::Module && motor(part.model)) {
            topology.emplace(doc);
            break;
        }
    auto read = [&](const std::string &id, const char *pin) -> std::optional<double> {
        auto it = state.voltage.find({id, pin});
        return it == state.voltage.end() ? std::nullopt : it->second;
    };
    for (const auto &p : doc.parts())
        if (p.kind == PartKind::Module && p.model == "a4988") {
            auto &r = memory[p.id];
            const auto g = read(p.id, "GND"), v = read(p.id, "VDD");
            auto high = [&](const char *pin) {
                auto x = read(p.id, pin);
                return g && v && x && *x - *g >= .7 * (*v - *g);
            };
            const bool edge = high("STEP") && !r[1];
            r[1] = high("STEP");
            r[6] = 0;
            r[2] = 0;
            if (now > r[4] && now - r[4] > 1000000)
                r[5] = 0;
            if (!g || !v || *v - *g < 3 || *v - *g > 5.5)
                continue;
            if (!high("RESET")) {
                r[0] = 0;
                continue;
            }
            const auto mg = read(p.id, "GND_MOT"), mv = read(p.id, "VMOT");
            if (!mg || !mv || *mv - *mg < 8 || *mv - *mg > 35 || !high("SLEEP") || high("ENABLE"))
                continue;
            const int mask = int(high("MS1")) | int(high("MS2")) * 2 | int(high("MS3")) * 4;
            const int modes[] = {1, 2, 4, 8, 0, 0, 0, 16};
            const int mode = modes[mask];
            r[3] = mode;
            if (!mode)
                continue;
            r[6] = 1;
            if (edge) {
                move(r, high("DIR") ? 16 / mode : -16 / mode);
                r[5] = r[4] && now > r[4] ? 1000000000ULL / (now - r[4]) / mode : 0;
                r[4] = now;
            }
            const double phase = position(r) * std::numbers::pi / 32 + std::numbers::pi / 4;
            r[2] = (std::cos(phase) > 0 ? 1 : 4) | (std::sin(phase) > 0 ? 2 : 8);
        }
    for (const auto &p : doc.parts())
        if (p.kind == PartKind::Module && motor(p.model)) {
            auto &r = memory[p.id];
            r[6] = 0;
            if (now > r[4] && now - r[4] > 1000000)
                r[5] = 0;
            bool assigned = false;
            if (p.model == "stepper-4" || p.model == "stepper-6")
                for (const auto &driver : doc.parts())
                    if (driver.kind == PartKind::Module && driver.model == "a4988" &&
                        topology->connected({p.id, "A1"}, {driver.id, "1A"}) &&
                        topology->connected({p.id, "A2"}, {driver.id, "1B"}) &&
                        topology->connected({p.id, "B1"}, {driver.id, "2A"}) &&
                        topology->connected({p.id, "B2"}, {driver.id, "2B"})) {
                        const auto &d = memory[driver.id];
                        if (d[6]) {
                            if (r[7])
                                r[0] += d[0] - r[8];
                            r[8] = d[0];
                            r[7] = 1;
                            r[6] = 1;
                            r[3] = d[3];
                            r[5] = d[5];
                            r[2] = d[2];
                        } else
                            r[7] = 0;
                        assigned = true;
                        break;
                    }
            if (assigned)
                continue;
            // Eight half-step coil states. Invalid/skipped sequences never invent motion.
            static constexpr int sequence[] = {1, 3, 2, 6, 4, 12, 8, 9};
            int mask = 0;
            auto diff = [&](const char *a, const char *b) {
                auto x = read(p.id, a), y = read(p.id, b);
                return x && y ? *x - *y : 0.;
            };
            if (p.model == "stepper-5")
                for (int i = 0; i < 4; ++i) {
                    auto pin = std::to_string(i + 1);
                    if (diff("COM", pin.c_str()) > 1)
                        mask |= 1 << i;
                }
            else if (p.model == "stepper-6") {
                if (diff("AC", "A1") > 1)
                    mask |= 1;
                if (diff("BC", "B1") > 1)
                    mask |= 2;
                if (diff("AC", "A2") > 1)
                    mask |= 4;
                if (diff("BC", "B2") > 1)
                    mask |= 8;
            } else {
                const double a = diff("A1", "A2"), b = diff("B1", "B2");
                if (a > 1)
                    mask |= 1;
                if (a < -1)
                    mask |= 4;
                if (b > 1)
                    mask |= 2;
                if (b < -1)
                    mask |= 8;
            }
            const auto it = std::find(std::begin(sequence), std::end(sequence), mask);
            if (it == std::end(sequence)) {
                r[7] = 0;
                continue;
            }
            const int phase = int(it - std::begin(sequence));
            r[6] = 1;
            r[2] = mask;
            if (r[7]) {
                int delta = (phase - int(r[8]) + 8) % 8;
                if (delta == 7)
                    delta = -1;
                else if (delta == 6)
                    delta = -2;
                if (delta >= -2 && delta <= 2 && delta != 0) {
                    move(r, delta * 8);
                    r[5] = r[4] && now > r[4] ? 1000000000ULL * std::abs(delta) / (now - r[4]) / 2 : 0;
                    r[4] = now;
                }
            }
            r[7] = 1;
            r[8] = phase;
            r[3] = (mask & (mask - 1)) ? 1 : 2;
        }
}
void describeActuators(const CircuitDocument &doc, const DeviceRegisters &memory, CircuitState &state) {
    for (const auto &p : doc.parts())
        if (p.kind == PartKind::Module) {
            if ((p.model == "a4988" || motor(p.model)) && memory.contains(p.id)) {
                const auto &r = memory.at(p.id);
                const double steps = position(r) / 16.;
                state.partValues[p.id] =
                    motor(p.model) ? steps * 360 / parameter(p, "steps", p.model == "stepper-5" ? 2048 : 200)
                                   : steps;
                state.partValues[p.id + "/steps"] = steps;
                state.partValues[p.id + "/microsteps"] = r[3];
                state.partValues[p.id + "/coils"] = r[2];
                state.partValues[p.id + "/enabled"] = r[6];
                state.partValues[p.id + "/stepRate"] = r[5] / 1000.;
                if (motor(p.model))
                    state.partValues[p.id + "/rpm"] =
                        r[5] * .06 / parameter(p, "steps", p.model == "stepper-5" ? 2048 : 200);
            } else if (p.model == "relay-5v") {
                const auto current = state.branchCurrent.find(p.id);
                (void)current;
                auto v = [&](const char *pin) -> std::optional<double> {
                    auto it = state.voltage.find({p.id, pin});
                    return it == state.voltage.end() ? std::nullopt : it->second;
                };
                auto supply = v("VCC"), ground = v("GND"), in = v("IN");
                bool on = false;
                if (supply && ground && in && *supply - *ground >= 4.5 && *supply - *ground <= 5.5)
                    on = parameter(p, "activeLow", 0) ? *in - *ground >= -.3 && *in - *ground <= .8
                                                      : *in - *ground >= 2.4 && *in <= *supply + .3;
                state.partValues[p.id] = on;
                state.ledCurrent[p.id] = on ? .003 : 0;
            }
        }
}
} // namespace thinkercad
