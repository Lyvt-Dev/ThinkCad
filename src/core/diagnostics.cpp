#include "thinkercad/diagnostics.hpp"
#include "thinkercad/modules.hpp"
#include <algorithm>
#include <cmath>
#include <set>
namespace thinkercad {
std::vector<DiagnosticIssue>
DiagnosticEngine::analyze(const CircuitDocument &doc, const NetTopology &net, const CircuitState &state,
                          const std::map<std::string, const Esp8266Board *> &boards, const UartBus *uart,
                          const BusTraffic *traffic) {
    std::vector<DiagnosticIssue> result;
    auto add = [&](std::string rule, Severity severity, Endpoint target, std::string explanation,
                   std::string solution) {
        result.push_back(
            {std::move(rule), std::move(explanation), std::move(solution), severity, std::move(target), {}});
    };
    auto voltage = [&](Endpoint ep) -> std::optional<double> {
        auto it = state.voltage.find(ep);
        return it == state.voltage.end() ? std::nullopt : it->second;
    };
    std::map<int, std::vector<std::pair<Endpoint, double>>> drivers;
    for (const auto &[id, board] : boards) {
        drivers[net.node({id, "GND"})].push_back({{id, "GND"}, 0});
        drivers[net.node({id, "3V3"})].push_back({{id, "3V3"}, 3.3});
        if (board->profile().arduino())
            drivers[net.node({id, "5V"})].push_back({{id, "5V"}, 5});
        for (int gpio : board->profile().gpio) {
            Endpoint ep{id, board->profile().pinName(gpio)};
            const auto &pin = board->pin(gpio);
            if (pin.mode == PinMode::Output && !pin.pwm && !pin.frequency)
                drivers[net.node(ep)].push_back({ep, pin.output ? board->profile().voltage : 0});
            else if (pin.mode == PinMode::Input && net.members(ep).size() > 1 && !voltage(ep))
                add("floating-input", Severity::Warning, ep,
                    "Digitaler Eingang hat keinen definierten Pegel.",
                    "Pullup/Pulldown oder einen aktiven Signaltreiber anschließen.");
        }
    }
    for (const auto &[node, list] : drivers)
        if (list.size() > 1) {
            auto [lo, hi] = std::minmax_element(
                list.begin(), list.end(), [](const auto &a, const auto &b) { return a.second < b.second; });
            if (hi->second - lo->second > .01)
                add("driver-conflict", Severity::Error, hi->first,
                    "Widersprüchliche Treiber oder Versorgung gegen GND kurzgeschlossen.",
                    "Versorgungsnetze trennen und niemals gegensätzliche Ausgänge verbinden.");
        }
    // Graph of finite conducting branches, excluding the LED under inspection.
    // This is reachability, not an invented ideal-ground requirement.
    std::map<int, std::vector<int>> graph;
    std::set<int> returns;
    for (const auto &[id, board] : boards) {
        (void)board;
        returns.insert(net.node({id, "GND"}));
    }
    for (const auto &p : doc.parts()) {
        if (p.kind == PartKind::BatteryAA || p.kind == PartKind::Battery3V || p.kind == PartKind::Battery9V ||
            p.kind == PartKind::PowerSupply) {
            returns.insert(net.node({p.id, "-"}));
            if (net.connected({p.id, "+"}, {p.id, "-"}))
                add("source-short", Severity::Error, {p.id, "+"},
                    "Plus- und Minuspol einer Spannungsquelle sind kurzgeschlossen.",
                    "Den direkten Kurzschluss entfernen und die Last in den Stromkreis einsetzen.");
        }
        if (p.kind == PartKind::Ground)
            returns.insert(net.node({p.id, "GND"}));
        const bool resistor = p.kind == PartKind::Resistor || p.kind == PartKind::Photoresistor ||
                              p.kind == PartKind::Thermistor;
        if (resistor) {
            int a = net.node({p.id, "1"}), b = net.node({p.id, "2"});
            graph[a].push_back(b);
            graph[b].push_back(a);
        }
    }
    std::set<int> returnReachable;
    std::vector<int> pending(returns.begin(), returns.end());
    while (!pending.empty()) {
        int node = pending.back();
        pending.pop_back();
        if (!returnReachable.insert(node).second)
            continue;
        auto edges = graph.find(node);
        if (edges != graph.end())
            pending.insert(pending.end(), edges->second.begin(), edges->second.end());
    }
    std::set<int> supplies;
    for (const auto &[node, list] : drivers)
        if (std::any_of(list.begin(), list.end(), [](const auto &d) { return d.second > 0; }))
            supplies.insert(node);
    for (const auto &p : doc.parts())
        if (p.kind == PartKind::Led || p.kind == PartKind::InfraredLed) {
            const auto a = net.node({p.id, "A"}), k = net.node({p.id, "K"});
            if (net.members({p.id, "K"}).size() > 1 && !voltage({p.id, "K"}) && !returnReachable.contains(k))
                add("led-return", Severity::Warning, {p.id, "K"},
                    "LED hat keinen erkennbaren elektrischen Rückweg.",
                    "Kathode über einen geschlossenen Stromkreis mit dem Minuspol verbinden.");
            auto current = state.ledCurrent.find(p.id);
            if (current != state.ledCurrent.end() &&
                current->second > (p.kind == PartKind::InfraredLed ? .05 : .02))
                add("led-overcurrent", Severity::Error, {p.id, "A"},
                    "LED-Strom überschreitet die Modellgrenze.",
                    "Geeigneten Vorwiderstand in Reihe einsetzen oder Versorgung reduzieren.");
            else if (supplies.contains(a) && returns.contains(k))
                add("led-limiter", Severity::Warning, {p.id, "A"},
                    "LED liegt ohne erkennbaren Strombegrenzer direkt an der Versorgung.",
                    "Geeigneten Vorwiderstand in Reihe einsetzen oder Versorgung reduzieren.");
        }
    std::map<std::tuple<int, int, int>, Endpoint> addresses;
    for (const auto &p : doc.parts()) {
        const bool module = p.kind == PartKind::Module;
        const ModuleDefinition *def = module ? &ModuleRegistry::get(p.model) : nullptr;
        const bool display = p.kind == PartKind::Oled || p.kind == PartKind::I2cCharacterLcd;
        const auto pins = CircuitDocument::pins(p);
        auto has = [&](const std::string &name) {
            return std::find(pins.begin(), pins.end(), name) != pins.end();
        };
        const std::string power = def ? def->power : "VCC", ground = def ? def->ground : "GND";
        if (has(power) && has(ground)) {
            auto v = voltage({p.id, power}), g = voltage({p.id, ground});
            const auto min = def ? def->minVoltage : p.kind == PartKind::Oled ? 3. : 2.7;
            const auto max = def ? def->maxVoltage : p.kind == PartKind::Oled ? 3.6 : 5.5;
            if (!v || !g || *v - *g < min || *v - *g > max)
                add("supply", Severity::Warning, {p.id, power},
                    "Versorgung fehlt oder liegt außerhalb des modellierten Bereichs.",
                    "VCC und GND sowie die zulässige Versorgungsspannung prüfen.");
        }
        if (display || (def && def->bus == "i2c")) {
            int address = display ? int(p.value) : int(parameter(p, "address", def->address));
            auto key = std::tuple{net.node({p.id, "SDA"}), net.node({p.id, "SCL"}), address};
            auto [it, inserted] = addresses.emplace(key, Endpoint{p.id, "SDA"});
            if (!inserted)
                add("i2c-address-conflict", Severity::Error, {p.id, "SDA"},
                    "Mehrere I²C-Geräte verwenden dieselbe Adresse am selben Bus.",
                    "Eine Geräteadresse ändern oder einen getrennten Bus verwenden.");
        }
    }
    if (uart) {
        std::map<int, std::vector<Endpoint>> tx, rx;
        for (const auto &[id, board] : boards)
            for (int port : UartBus::supportedPorts(board->profile()))
                if (uart->active(id, port)) {
                    auto c = uart->config(id, port);
                    if (c.tx >= 0) {
                        Endpoint e{id, board->profile().pinName(c.tx)};
                        tx[net.node(e)].push_back(e);
                    }
                    if (c.rx >= 0) {
                        Endpoint e{id, board->profile().pinName(c.rx)};
                        rx[net.node(e)].push_back(e);
                    }
                }
        for (const auto &[node, list] : tx)
            if (list.size() > 1)
                add("uart-tx", Severity::Error, list.front(),
                    "Mehrere aktive UART-TX-Pins sind miteinander verbunden.",
                    "TX des einen Boards mit RX des anderen verbinden und GND verbinden.");
        for (const auto &[node, list] : rx)
            if (list.size() > 1 && !tx.contains(node))
                add("uart-rx", Severity::Warning, list.front(),
                    "UART-RX-Pins sind ohne erkennbaren TX-Treiber verbunden.",
                    "TX des einen Boards mit RX des anderen verbinden und GND verbinden.");
    }
    // Protocol errors remain evidence from actual operations, not sketch parsing.
    if (traffic)
        for (auto it = traffic->entries().rbegin(); it != traffic->entries().rend(); ++it)
            if (it->bus == "I2C" && it->status.starts_with("NACK")) {
                if (boards.contains(it->board)) {
                    auto profile = boards.at(it->board)->profile();
                    add("i2c-access", it->txBytes || it->rxBytes ? Severity::Warning : Severity::Info,
                        {it->board, profile.pinName(profile.sda)}, "Ein I²C-Zugriff wurde nicht bestätigt.",
                        "Geräteadresse, Versorgung, SDA/SCL und Buskonfiguration prüfen.");
                }
                break;
            }
    return result;
}
} // namespace thinkercad
