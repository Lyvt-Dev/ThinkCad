#include "thinkercad/bme680.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
// Bosch's published compensation code is also used by the native sketch driver.
// Keep its license and pinned provenance in sdk/vendor/bme680.
extern "C" {
#include "bme68x.c"
}
namespace thinkercad {
namespace {
bme68x_dev calibrated() {
    bme68x_dev d{};
    auto &c = d.calib;
    c.par_t1 = 25925;
    c.par_t2 = 26470;
    c.par_t3 = 3;
    c.par_p1 = 36467;
    c.par_p2 = -10685;
    c.par_p3 = 88;
    c.par_p4 = 10435;
    c.par_p5 = -297;
    c.par_p6 = 30;
    c.par_p7 = 41;
    c.par_p8 = -10230;
    c.par_p9 = -3385;
    c.par_p10 = 30;
    c.par_h1 = 784;
    c.par_h2 = 1022;
    c.par_h3 = 0;
    c.par_h4 = 45;
    c.par_h5 = 20;
    c.par_h6 = 120;
    c.par_h7 = -100;
    c.par_gh1 = -30;
    c.par_gh2 = -2500;
    c.par_gh3 = 4;
    c.res_heat_range = 1;
    c.res_heat_val = -20;
    c.range_sw_err = 0;
    return d;
}
template <class F> unsigned inverse(unsigned maximum, double target, F convert, bool ascending) {
    unsigned lo = 0, hi = maximum;
    while (lo < hi) {
        const auto mid = lo + (hi - lo) / 2;
        if ((convert(mid) < target) == ascending)
            lo = mid + 1;
        else
            hi = mid;
    }
    if (lo && std::abs(convert(lo - 1) - target) < std::abs(convert(lo) - target))
        --lo;
    return lo;
}
} // namespace
std::array<std::uint8_t, 256> bme680Calibration() {
    const auto device = calibrated();
    const auto &c = device.calib;
    std::array<uint8_t, 42> coefficients{};
    auto word = [&](int index, int value) {
        coefficients[index] = uint16_t(value) & 255;
        coefficients[index + 1] = uint16_t(value) >> 8;
    };
    word(BME68X_IDX_T1_LSB, c.par_t1);
    word(BME68X_IDX_T2_LSB, c.par_t2);
    coefficients[BME68X_IDX_T3] = c.par_t3;
    word(BME68X_IDX_P1_LSB, c.par_p1);
    word(BME68X_IDX_P2_LSB, c.par_p2);
    coefficients[BME68X_IDX_P3] = c.par_p3;
    word(BME68X_IDX_P4_LSB, c.par_p4);
    word(BME68X_IDX_P5_LSB, c.par_p5);
    coefficients[BME68X_IDX_P6] = c.par_p6;
    coefficients[BME68X_IDX_P7] = c.par_p7;
    word(BME68X_IDX_P8_LSB, c.par_p8);
    word(BME68X_IDX_P9_LSB, c.par_p9);
    coefficients[BME68X_IDX_P10] = c.par_p10;
    coefficients[BME68X_IDX_H1_MSB] = c.par_h1 >> 4;
    coefficients[BME68X_IDX_H2_MSB] = c.par_h2 >> 4;
    coefficients[BME68X_IDX_H1_LSB] = (c.par_h1 & 15) | ((c.par_h2 & 15) << 4);
    coefficients[BME68X_IDX_H3] = c.par_h3;
    coefficients[BME68X_IDX_H4] = c.par_h4;
    coefficients[BME68X_IDX_H5] = c.par_h5;
    coefficients[BME68X_IDX_H6] = c.par_h6;
    coefficients[BME68X_IDX_H7] = c.par_h7;
    coefficients[BME68X_IDX_GH1] = c.par_gh1;
    word(BME68X_IDX_GH2_LSB, c.par_gh2);
    coefficients[BME68X_IDX_GH3] = c.par_gh3;
    coefficients[BME68X_IDX_RES_HEAT_RANGE] = c.res_heat_range << 4;
    coefficients[BME68X_IDX_RES_HEAT_VAL] = c.res_heat_val;
    std::array<uint8_t, 256> registers{};
    for (int i = 0; i < 23; ++i)
        registers[0x8a + i] = coefficients[i];
    for (int i = 0; i < 14; ++i)
        registers[0xe1 + i] = coefficients[23 + i];
    for (int i = 0; i < 5; ++i)
        registers[i] = coefficients[37 + i];
    registers[0xd0] = 0x61;
    registers[0xf0] = 0;
    return registers;
}
Microseconds bme680Duration(const std::array<uint8_t, 256> &registers) {
    auto device = calibrated();
    bme68x_conf configuration{};
    configuration.os_temp = (registers[0x74] >> 5) & 7;
    configuration.os_pres = (registers[0x74] >> 2) & 7;
    configuration.os_hum = registers[0x72] & 7;
    auto duration = bme68x_get_meas_dur(BME68X_FORCED_MODE, &configuration, &device);
    if (registers[0x71] & 0x10) {
        const unsigned wait = registers[0x64];
        duration += 1000ULL * (wait & 63) * (1u << (2 * (wait >> 6)));
    }
    return duration;
}
void sampleBme680(const CircuitPart &part, std::array<uint8_t, 256> &registers, Microseconds now) {
    auto device = calibrated();
    const double seconds = now * 1e-6, variation = parameter(part, "variation", 0);
    const double temperature =
        std::clamp(parameter(part, "temperature", 25) + variation * .25 * std::sin(seconds / 37), -40., 85.);
    const double pressure =
        std::clamp(parameter(part, "pressure", 1013.25) + variation * .3 * std::sin(seconds / 97), 300.,
                   1100.) *
        100;
    const double humidity =
        std::clamp(parameter(part, "humidity", 50) + variation * 1.5 * std::sin(seconds / 53), 0., 100.);
    const double gas =
        parameter(part, "gasResistance", 100000) * (1 + variation * .025 * std::sin(seconds / 25));
    const auto rawTemperature =
        inverse(0xfffff, temperature, [&](unsigned raw) { return calc_temperature(raw, &device); }, true);
    calc_temperature(rawTemperature, &device);
    const auto rawPressure =
        inverse(0xfffff, pressure, [&](unsigned raw) { return calc_pressure(raw, &device); }, false);
    const auto rawHumidity =
        inverse(65535, humidity, [&](unsigned raw) { return calc_humidity(raw, &device); }, true);
    unsigned rawGas = 0, gasRange = 0;
    double error = std::numeric_limits<double>::infinity();
    for (unsigned range = 0; range < 16; ++range) {
        const auto raw = inverse(
            1023, gas, [&](unsigned adc) { return calc_gas_resistance_low(adc, range, &device); }, false);
        const auto candidate = std::abs(calc_gas_resistance_low(raw, range, &device) - gas);
        if (candidate < error) {
            error = candidate;
            rawGas = raw;
            gasRange = range;
        }
    }
    registers[0x1d] = 0x80;
    ++registers[0x1e];
    registers[0x1f] = rawPressure >> 12;
    registers[0x20] = rawPressure >> 4;
    registers[0x21] = rawPressure << 4;
    registers[0x22] = rawTemperature >> 12;
    registers[0x23] = rawTemperature >> 4;
    registers[0x24] = rawTemperature << 4;
    registers[0x25] = rawHumidity >> 8;
    registers[0x26] = rawHumidity;
    registers[0x2a] = rawGas >> 2;
    registers[0x2b] = ((rawGas & 3) << 6) | gasRange | ((registers[0x71] & 0x10) ? 0x30 : 0);
}
} // namespace thinkercad
