#include "thinkercad/moduledevices.hpp"
#include "thinkercad/uartbus.hpp"
#include "thinkercad/diagnostics.hpp"
#include "bme68x.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace thinkercad;
static void check(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}
struct SensorContext {
    ModuleDevices devices;
    CircuitPart part;
    Microseconds now = 0;
};
static int8_t readSensor(uint8_t address, uint8_t *output, uint32_t size, void *context) {
    auto &c = *static_cast<SensorContext *>(context);
    c.devices.i2c(c.part, {address}, -1, c.now);
    const auto bytes = c.devices.i2c(c.part, {}, int(size), c.now);
    std::copy(bytes.begin(), bytes.end(), output);
    return 0;
}
static int8_t writeSensor(uint8_t address, const uint8_t *data, uint32_t size, void *context) {
    auto &c = *static_cast<SensorContext *>(context);
    std::vector<uint8_t> bytes{address};
    bytes.insert(bytes.end(), data, data + size);
    c.devices.i2c(c.part, bytes, -1, c.now);
    return 0;
}
static void delaySensor(uint32_t duration, void *context) {
    static_cast<SensorContext *>(context)->now += duration;
}
int main() {
    try {
        SensorContext context;
        context.part.id = "environment";
        context.part.kind = PartKind::Module;
        context.part.model = "bme680";
        bme68x_dev device{};
        device.intf = BME68X_I2C_INTF;
        device.read = readSensor;
        device.write = writeSensor;
        device.delay_us = delaySensor;
        device.intf_ptr = &context;
        device.amb_temp = 25;
        check(bme68x_init(&device) == BME68X_OK,
              "Real Bosch driver must recognize BME680 chip and calibration");
        bme68x_conf configuration{};
        configuration.os_temp = BME68X_OS_8X;
        configuration.os_pres = BME68X_OS_4X;
        configuration.os_hum = BME68X_OS_2X;
        check(bme68x_set_conf(&configuration, &device) == BME68X_OK, "Bosch oversampling configuration");
        bme68x_heatr_conf heater{};
        heater.enable = BME68X_ENABLE;
        heater.heatr_temp = 320;
        heater.heatr_dur = 150;
        check(bme68x_set_heatr_conf(BME68X_FORCED_MODE, &heater, &device) == BME68X_OK,
              "Gas heater configuration");
        for (double temperature : {-30., 0., 25., 60., 80.}) {
            context.part.parameters = {{"temperature", temperature},
                                       {"humidity", 55},
                                       {"pressure", 1000},
                                       {"gasResistance", 123456}};
            check(bme68x_set_op_mode(BME68X_FORCED_MODE, &device) == BME68X_OK, "Start forced measurement");
            auto before = context.devices.registers(context.part, context.now);
            check(!(before[0x1d] & 0x80), "Conversion must not complete instantly");
            context.now += 1000000;
            bme68x_data data{};
            uint8_t count = 0;
            check(bme68x_get_data(BME68X_FORCED_MODE, &data, &count, &device) == BME68X_OK && count == 1,
                  "Forced conversion returns one sample");
            check(std::abs(data.temperature - temperature) < .03,
                  "Temperature through real Bosch compensation");
            check(std::abs(data.humidity - 55) < .1, "Humidity through real Bosch compensation");
            check(std::abs(data.pressure - 100000) < 3, "Pressure through real Bosch compensation");
            check(std::abs(double(data.gas_resistance) - 123456) / 123456 < .01,
                  "Quantized simulated gas resistance");
            check((data.status & BME68X_GASM_VALID_MSK) != 0,
                  "Gas validity status after modeled heater delay");
        }
        CircuitDocument doc;
        doc.part("board").model = "esp32-s3";
        Esp8266Board board;
        board.configure("esp32-s3");
        auto sensor = doc.addPart(PartKind::Module, 0, 0);
        doc.part(sensor).model = "mh-z19b";
        doc.part(sensor).parameters["co2"] = 812;
        doc.connect({sensor, "VCC"}, {"board", "5V"});
        doc.connect({sensor, "GND"}, {"board", "GND"});
        doc.connect({sensor, "RX"}, {"board", "GPIO18"});
        const auto rxWire = doc.connect({sensor, "TX"}, {"board", "GPIO17"});
        board.pinMode(18, PinMode::Output);
        board.digitalWrite(18, true);
        board.pinMode(17, PinMode::Input);
        UartBus uart;
        uart.begin(doc, "board", 1, {9600, 17, 18, 8, 1, UartBus::Parity::None});
        auto write = [&](const std::vector<uint8_t> &bytes, Microseconds now) {
            uart.write(doc, doc.solve(board), "board", 1, bytes, now, true, {{"board", &board}});
        };
        const std::vector<uint8_t> request{0xff, 1, 0x86, 0, 0, 0, 0, 0, 0x79};
        write({request.begin(), request.begin() + 4}, 0);
        write({request.begin() + 4, request.end()}, 5000);
        check(uart.available("board", 1, 50000) == 0, "Sensor response delay");
        check(uart.available("board", 1, 200000) == 9, "Fragmented MH-Z19 command response");
        std::vector<uint8_t> response;
        while (uart.available("board", 1, 200000))
            response.push_back(uint8_t(uart.read("board", 1, 200000)));
        check(response[1] == 0x86 && int(response[2]) * 256 + response[3] == 812, "CO2 uses inspector value");
        unsigned sum = 0;
        for (int i = 1; i <= 8; ++i)
            sum += response[i];
        check((sum & 255) == 0, "MH-Z19 response checksum");
        auto bad = request;
        bad[8] ^= 1;
        write(bad, 300000);
        check(uart.available("board", 1, 500000) == 0, "Bad command checksum must be ignored");
        doc.part(sensor).parameters["faultChecksum"] = 1;
        write(request, 600000);
        response.clear();
        while (uart.available("board", 1, 900000))
            response.push_back(uint8_t(uart.read("board", 1, 900000)));
        check(response.size() == 9, "Checksum fault retains response length");
        sum = 0;
        for (int i = 1; i <= 8; ++i)
            sum += response[i];
        check((sum & 255) != 0, "Injected bad response checksum");
        auto issues = DiagnosticEngine::analyze(doc, NetTopology(doc), doc.solve(board), {{"board", &board}});
        check(std::any_of(issues.begin(), issues.end(),
                          [&](const auto &d) {
                              return d.rule == "sensor-fault" && d.target.part == sensor &&
                                     !d.solution.empty();
                          }),
              "Simulated checksum fault has actionable diagnosis");
        doc.part(sensor).parameters["faultNoResponse"] = 1;
        write(request, 1000000);
        check(uart.available("board", 1, 2000000) == 0, "No-response fault");
        doc.part(sensor).parameters["faultNoResponse"] = 0;
        doc.removeWire(rxWire);
        write(request, 2100000);
        check(uart.available("board", 1, 3000000) == 0, "Missing sensor TX wire");
        doc.connect({sensor, "TX"}, {"board", "GPIO17"});
        write(request, 3100000);
        for (const auto &wire : std::vector<CircuitWire>(doc.wires()))
            if ((wire.a == Endpoint{sensor, "VCC"}) || (wire.b == Endpoint{sensor, "VCC"}))
                doc.removeWire(wire.id);
        uart.synchronizeDevices(doc, doc.solve(board), 3120000);
        check(uart.available("board", 1, 4000000) == 0, "Power loss cancels pending sensor responses");
        std::cout << "BME680 Bosch-driver compensation and MH-Z19 UART timing/faults passed\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
