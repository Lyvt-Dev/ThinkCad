#include "thinkercad/tft.hpp"
#include "thinkercad/spibus.hpp"
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
        for (bool ili : {true, false}) {
            TftController tft(ili ? 240 : 128, ili ? 320 : 160, ili);
            auto command = [&](uint8_t c, std::initializer_list<uint8_t> data, Microseconds time = 120000) {
                tft.transfer(false, c, time);
                for (auto v : data)
                    tft.transfer(true, v, time);
            };
            check(!tft.frame(0, 1), "Uninitialized panel must stay off");
            command(0x11, {}, 0);
            command(0x3a, {0x55}, 0);
            command(0x29, {}, 0);
            check(!tft.frame(119999, 1), "Sleep-out delay is simulated");
            command(0x2a, {0, 0, 0, 1});
            command(0x2b, {0, 0, 0, 0});
            command(0x2c, {0xf8, 0, 0x07, 0xe0});
            auto frame = tft.frame(120000, 1);
            check(frame && frame->pixels[0] == 0xffff0000 && frame->pixels[1] == 0xff00ff00,
                  "RGB565 red and green");
            check(frame == tft.frame(120000, 1), "Unchanged frame must share cached storage");
            check(!tft.frame(120000, 0), "Backlight must be connected");
            command(0x21, {});
            frame = tft.frame(120000, 1);
            check(frame->pixels[0] == 0xff00ffff, "Display inversion");
            command(0x20, {});
            command(0x36, {0x28});
            command(0x2a, {0, 0, 0, 0});
            command(0x2b, {0, 1, 0, 1});
            command(0x2c, {0, 0x1f});
            frame = tft.frame(120000, 1);
            check(frame->pixels[1] == 0xff0000ff, "MADCTL swaps axes");
            command(0x2a, {0xff, 0xff, 0xff, 0xff});
            check(!tft.error().empty(), "Bad address window is diagnosed");
            command(0x28, {});
            check(!tft.frame(120000, 1), "Display-off command");
            tft.reset();
            check(!tft.frame(120000, 1), "Hardware reset clears initialized state");
        }
        CircuitDocument doc;
        doc.part("board").model = "esp32-s3";
        Esp8266Board board;
        board.configure("esp32-s3");
        auto panel = doc.addPart(PartKind::Module, 0, 0);
        doc.part(panel).model = "ili9341";
        doc.connect({panel, "VCC"}, {"board", "3V3"});
        doc.connect({panel, "GND"}, {"board", "GND"});
        doc.connect({panel, "LED"}, {"board", "3V3"});
        doc.connect({panel, "RESET"}, {"board", "GPIO8"});
        doc.connect({panel, "DC"}, {"board", "GPIO9"});
        doc.connect({panel, "CS"}, {"board", "GPIO10"});
        doc.connect({panel, "SCK"}, {"board", "GPIO12"});
        const auto mosi = doc.connect({panel, "MOSI"}, {"board", "GPIO11"});
        for (int gpio : {8, 9, 10, 11, 12})
            board.pinMode(gpio, PinMode::Output);
        board.digitalWrite(8, true);
        ModuleDevices devices;
        SpiBus bus(devices);
        auto send = [&](bool data, const std::vector<uint8_t> &bytes, Microseconds time) {
            board.digitalWrite(9, data);
            auto state = doc.solve(board);
            return bus.writeBlock(doc, state, "board", 12, 13, 11, 8000000, 0, false, bytes, time);
        };
        check(send(false, {0x11}, 0).status == SpiBus::Status::Ok,
              "Wired panel accepts commands without optional MISO");
        send(false, {0x3a}, 120000);
        send(true, {0x55}, 120000);
        send(false, {0x29}, 120000);
        send(false, {0x2a}, 120000);
        send(true, {0, 0, 0, 0}, 120000);
        send(false, {0x2b}, 120000);
        send(true, {0, 0, 0, 0}, 120000);
        send(false, {0x2c}, 120000);
        send(true, {0xf8, 0}, 120000);
        auto state = doc.solve(board);
        devices.applyDisplayFrames(doc, state, 120010);
        check(state.colorDisplays.at(panel)->pixels[0] == 0xffff0000,
              "SPI bytes reach physical display frame");
        doc.removeWire(mosi);
        check(send(false, {0x2c}, 130000).status == SpiBus::Status::Unselected,
              "Missing MOSI rejects transfers");
        board.digitalWrite(8, false);
        state = doc.solve(board);
        devices.applyDisplayFrames(doc, state, 130000);
        check(state.colorDisplays.empty(), "RESET low removes the visible frame");
        std::cout << "TFT initialization, RGB565, rotation, power and SPI wiring passed\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
