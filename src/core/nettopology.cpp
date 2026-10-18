#include "thinkercad/nettopology.hpp"
#include <algorithm>
#include <numeric>
#include <sstream>
namespace thinkercad {
NetTopology::NetTopology(const CircuitDocument &doc) {
    for (const auto &p : doc.parts())
        for (const auto &pin : CircuitDocument::pins(p))
            index_.emplace(Endpoint{p.id, pin}, static_cast<int>(index_.size()));
    parent_.resize(index_.size());
    std::iota(parent_.begin(), parent_.end(), 0);
    for (const auto &part : doc.parts())
        if (part.kind == PartKind::Esp8266) {
            const auto profile = BoardProfile::get(part.model);
            if (!profile.arduino() && profile.id != "esp8266")
                join({part.id, "A0"}, {part.id, profile.pinName(profile.defaultAdc)});
            join({part.id, "GND"}, {part.id, "GND2"});
            if (!profile.arduino())
                join({part.id, "3V3"}, {part.id, "3V3_2"});
        }
    for (const auto &w : doc.wires())
        join(w.a, w.b);
    std::map<std::string, Endpoint> named;
    for (const auto &p : doc.parts())
        if (p.kind == PartKind::NetLabel && !p.netName.empty()) {
            auto [it, inserted] = named.emplace(p.netName, Endpoint{p.id, "NET"});
            if (!inserted)
                join(it->second, {p.id, "NET"});
        }
    for (const auto &p : doc.parts()) {
        if ((p.kind == PartKind::ToggleSwitch || p.kind == PartKind::Button ||
             p.kind == PartKind::TiltSwitch || p.kind == PartKind::ReedSwitch) &&
            p.pressed)
            join({p.id, "1"}, {p.id, "2"});
        if (p.kind == PartKind::RotaryEncoder) {
            const int phase = parameter(p, "phase", 0);
            if (phase == 1 || phase == 2)
                join({p.id, "A"}, {p.id, "GND"});
            if (phase == 2 || phase == 3)
                join({p.id, "B"}, {p.id, "GND"});
            if (parameter(p, "switch", 0))
                join({p.id, "SW"}, {p.id, "GND"});
        }
        if (p.kind == PartKind::Keypad || p.kind == PartKind::Keyboard) {
            const auto mask = static_cast<uint32_t>(parameter(p, "keys", 0));
            const int columns = p.kind == PartKind::Keypad ? 4 : 8;
            for (int key = 0; key < (p.kind == PartKind::Keypad ? 16 : 25); ++key)
                if (mask & (1u << key))
                    join({p.id, "R" + std::to_string(key / columns + 1)},
                         {p.id, "C" + std::to_string(key % columns + 1)});
        }
        if (p.kind == PartKind::SlideSwitch)
            join({p.id, "C"}, {p.id, p.pressed ? "2" : "1"});
        if (p.kind == PartKind::Breadboard30 && p.model == "power-rails")
            for (const auto &rail : {"T+", "T-", "B+", "B-"})
                for (int n = 2; n <= 25; ++n)
                    join({p.id, std::string(rail) + "1"}, {p.id, std::string(rail) + std::to_string(n)});
        if (p.kind == PartKind::Breadboard30)
            for (int n = 1; n <= 30; ++n)
                for (char start : {'a', 'f'})
                    for (int j = 1; j < 5; ++j)
                        join({p.id, std::string(1, start) + std::to_string(n)},
                             {p.id, std::string(1, start + j) + std::to_string(n)});
    }
    for (std::size_t i = 0; i < parent_.size(); ++i)
        parent_[i] = find(static_cast<int>(i));
    for (const auto &[ep, i] : index_)
        members_[parent_[i]].push_back(ep);
    for (const auto &wire : doc.wires())
        wires_[node(wire.a)].push_back(wire.id);
}

const std::vector<std::string> &NetTopology::wires(const Endpoint &ep) const {
    static const std::vector<std::string> empty;
    auto it = wires_.find(node(ep));
    return it == wires_.end() ? empty : it->second;
}
std::vector<Endpoint> NetTopology::preview(const Endpoint &a, const Endpoint &b) const {
    auto result = members(a);
    if (!connected(a, b)) {
        const auto &other = members(b);
        result.insert(result.end(), other.begin(), other.end());
        std::sort(result.begin(), result.end());
    }
    return result;
}
std::string NetTopology::signature(const CircuitDocument &doc) {
    std::ostringstream out;
    auto field = [&](const std::string &s) { out << s.size() << ':' << s; };
    for (const auto &p : doc.parts()) {
        field(p.id);
        out << int(p.kind) << ';';
        field(p.model);
        field(p.netName);
        out << p.pressed << ';';
        for (const auto &[k, v] : p.parameters) {
            field(k);
            out.precision(17);
            out << v << ';';
        }
    }
    for (const auto &w : doc.wires()) {
        field(w.id);
        field(w.a.part);
        field(w.a.pin);
        field(w.b.part);
        field(w.b.pin);
    }
    return out.str();
}
} // namespace thinkercad
