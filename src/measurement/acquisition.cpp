#include "thinkercad/measurement/acquisition.hpp"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <locale>
#include <sstream>
#include <stdexcept>
namespace thinkercad::measurement {
TraceStore::TraceStore(std::size_t capacity) : capacity_(std::clamp<std::size_t>(capacity, 2, 1000000)) {}
void TraceStore::append(Sample value) {
    if (!data_.empty() && value.time < data_.back().time)
        throw std::invalid_argument("Trace time must be monotonic");
    if (value.channels.size() > 8)
        throw std::invalid_argument("Too many trace channels");
    for (auto &v : value.channels)
        if (v && !std::isfinite(*v))
            v.reset();
    if (!data_.empty() && data_.back().time == value.time)
        data_.back() = std::move(value);
    else {
        if (data_.size() == capacity_) {
            data_.pop_front();
            ++overwritten_;
        }
        data_.push_back(std::move(value));
    }
}
std::optional<double> TraceStore::valueAt(Microseconds time, std::size_t channel) const {
    if (data_.empty() || time < data_.front().time || time > data_.back().time)
        return {};
    auto it = std::upper_bound(data_.begin(), data_.end(), time,
                               [](auto t, const Sample &p) { return t < p.time; });
    --it;
    return channel < it->channels.size() ? it->channels[channel] : std::nullopt;
}
Sample acquire(const CircuitPart &part, Microseconds time, const CircuitState &state) {
    Sample sample{time, {}};
    auto voltage = [&](const std::string &pin) -> std::optional<double> {
        auto it = state.voltage.find({part.id, pin});
        return it == state.voltage.end() ? std::nullopt : it->second;
    };
    if (part.kind == PartKind::CurrentProbe) {
        auto i = state.branchCurrent.find(part.id);
        sample.channels.push_back(i == state.branchCurrent.end() ? std::nullopt : i->second);
        return sample;
    }
    auto ground = voltage("GND");
    const bool digital = part.kind == PartKind::LogicAnalyzer;
    for (int i = 0; i < (digital ? 8 : 2); ++i) {
        auto v = voltage(digital ? "D" + std::to_string(i) : i ? "B" : "A");
        if (v && ground) {
            *v -= *ground;
            if (digital)
                *v = *v >= part.value ? 1 : 0;
        } else
            v.reset();
        sample.channels.push_back(v);
    }
    return sample;
}
bool crossed(const Sample &a, const Sample &b, std::size_t channel, double threshold, bool rising) {
    if (b.time <= a.time || channel >= a.channels.size() || channel >= b.channels.size() ||
        !a.channels[channel] || !b.channels[channel])
        return false;
    return rising ? *a.channels[channel] < threshold && *b.channels[channel] >= threshold
                  : *a.channels[channel] > threshold && *b.channels[channel] <= threshold;
}
std::string csv(const std::deque<Sample> &samples, const std::vector<std::string> &columns) {
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << "time_s";
    for (const auto &column : columns) {
        out << ',';
        const bool quote = column.find_first_of(",\"\r\n") != std::string::npos;
        if (quote)
            out << '\"';
        for (char c : column) {
            if (c == '\"')
                out << '\"';
            out << c;
        }
        if (quote)
            out << '\"';
    }
    out << '\n' << std::setprecision(12);
    for (const auto &point : samples) {
        out << point.time * 1e-6;
        for (std::size_t i = 0; i < columns.size(); ++i) {
            out << ',';
            if (i < point.channels.size() && point.channels[i] && std::isfinite(*point.channels[i]))
                out << *point.channels[i];
        }
        out << '\n';
    }
    return out.str();
}
} // namespace thinkercad::measurement
