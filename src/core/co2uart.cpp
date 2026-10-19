#include "thinkercad/uartbus.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <set>
namespace thinkercad {
namespace {
bool powered(const CircuitPart &p, const CircuitState &state) {
    const auto v = state.voltage.find({p.id, "VCC"}), g = state.voltage.find({p.id, "GND"});
    return v != state.voltage.end() && g != state.voltage.end() && v->second && g->second &&
           *v->second - *g->second >= 4.5 && *v->second - *g->second <= 5.5;
}
std::uint8_t checksum(const std::vector<std::uint8_t> &frame) {
    unsigned sum = 0;
    for (int i = 1; i < 8; ++i)
        sum += frame[i];
    return std::uint8_t(0x100 - sum);
}
} // namespace
void UartBus::synchronizeDevices(const CircuitDocument &doc, const CircuitState &state, Microseconds now) {
    std::set<std::string> present;
    for (const auto &p : doc.parts())
        if (p.kind == PartKind::Module && p.model == "mh-z19b" && powered(p, state))
            present.insert(p.id);
    std::erase_if(sensors_, [&](const auto &item) { return !present.contains(item.first); });
    for (auto &[key, port] : ports_)
        std::erase_if(port.rx, [&](const auto &byte) {
            return byte.source.second == -1 && !present.contains(byte.source.first) && byte.ready > now;
        });
}
void UartBus::receiveSensors(const CircuitDocument &doc, const CircuitState &state, const std::string &board,
                             int port, const std::vector<std::uint8_t> &bytes, Microseconds now,
                             const std::map<std::string, const Esp8266Board *> &boards) {
    auto &source = require(board, port);
    const auto profile = BoardProfile::get(doc.part(board).model);
    const Endpoint tx{board, profile.pinName(source.config.tx)};
    for (const auto &sensor : doc.parts())
        if (sensor.kind == PartKind::Module && sensor.model == "mh-z19b" && powered(sensor, state)) {
            bool path =
                doc.connected(tx, {sensor.id, "RX"}) && doc.connected({board, "GND"}, {sensor.id, "GND"});
            if (!path)
                path = wirePath(doc, boards, board, source.config.tx, {sensor.id, "RX"});
            if (!path)
                continue;
            const auto received = state.voltage.find({sensor.id, "RX"}),
                       ground = state.voltage.find({sensor.id, "GND"});
            const bool validLevel = received != state.voltage.end() && ground != state.voltage.end() &&
                                    received->second && ground->second &&
                                    *received->second - *ground->second >= 2.5 &&
                                    *received->second - *ground->second <= 3.6;
            if (!validLevel || std::abs(double(source.config.baud) / 9600 - 1) > .025 ||
                source.config.dataBits != 8 || source.config.stopBits != 1 ||
                source.config.parity != Parity::None) {
                ++source.errors.framing;
                continue;
            }
            auto &device = sensors_[sensor.id];
            for (auto byte : bytes) {
                if (device.command.empty() && byte != 0xff)
                    continue;
                device.command.push_back(byte);
                if (device.command.size() == 2 && byte != 1) {
                    device.command.clear();
                    if (byte == 0xff)
                        device.command.push_back(byte);
                    continue;
                }
                if (device.command.size() < 9)
                    continue;
                const auto command = device.command;
                device.command.clear();
                if (command[8] != checksum(command) || command[2] != 0x86 ||
                    parameter(sensor, "faultNoResponse", 0))
                    continue;
                const int ppm = std::clamp(
                    int(std::lround(parameter(sensor, "co2", 400) +
                                    parameter(sensor, "variation", 0) * 15 * std::sin(now * 1e-6 / 61))),
                    0, 10000);
                std::vector<uint8_t> response{
                    0xff,
                    0x86,
                    uint8_t(ppm >> 8),
                    uint8_t(ppm),
                    uint8_t(std::clamp(int(std::lround(parameter(sensor, "temperature", 25))) + 40, 0, 255)),
                    0,
                    0,
                    0,
                    0};
                response[8] = checksum(response);
                if (parameter(sensor, "faultChecksum", 0))
                    response[8] ^= 1;
                const Microseconds delay = Microseconds(parameter(sensor, "responseMs", 100) * 1000);
                if (now > std::numeric_limits<Microseconds>::max() - delay - 20000)
                    continue;
                if (device.nextResponse > now && device.nextResponse - now > 10000000) {
                    ++source.errors.overrun;
                    continue;
                }
                auto at = std::max(now + delay, device.nextResponse);
                constexpr Microseconds duration = 1042; // 8N1 at 9600 baud, rounded up.
                for (const auto value : response) {
                    const auto ready = at + duration;
                    for (auto &[key, receiver] : ports_) {
                        if (receiver.config.rx < 0 ||
                            !doc.connected(
                                {sensor.id, "TX"},
                                {key.first,
                                 BoardProfile::get(doc.part(key.first).model).pinName(receiver.config.rx)}) ||
                            !doc.connected({sensor.id, "GND"}, {key.first, "GND"}))
                            continue;
                        const auto &config = receiver.config;
                        if (std::abs(double(config.baud) / 9600 - 1) > .025 || config.dataBits != 8 ||
                            config.stopBits != 1 || config.parity != Parity::None) {
                            ++receiver.errors.framing;
                            continue;
                        }
                        Byte incoming{at, ready, value, {sensor.id, -1}, true};
                        for (auto &pending : receiver.rx)
                            if (pending.source != incoming.source && pending.ready > at &&
                                pending.start < ready) {
                                if (pending.valid)
                                    ++receiver.errors.collision;
                                pending.valid = false;
                                incoming.valid = false;
                            }
                        if (!incoming.valid)
                            ++receiver.errors.collision;
                        if (receiver.rx.size() >= 256) {
                            ++receiver.errors.overrun;
                            continue;
                        }
                        const auto location = std::upper_bound(
                            receiver.rx.begin(), receiver.rx.end(), ready,
                            [](auto time, const auto &pending) { return time < pending.ready; });
                        receiver.rx.insert(location, std::move(incoming));
                    }
                    at = ready;
                }
                device.nextResponse = at;
            }
        }
}
} // namespace thinkercad
