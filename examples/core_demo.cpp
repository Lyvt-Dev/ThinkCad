#include "thinkercad/simulation.hpp"
#include <iostream>
using namespace thinkercad;
int main() {
    Simulator sim; Esp8266Board board; OledDisplay display; Led led("red-led");
    board.pinMode(2, PinMode::Output);
    Animation animation;
    for (int i=0; i<3; ++i) { Frame frame; frame.setPixel(i,32,true); animation.addFrame(std::move(frame),100000); }
    animation.enqueue(sim, display, 0);
    sim.schedule(50000, [&] { board.digitalWrite(2,true); led.apply(3.3,0); });
    sim.runUntil(250000);
    std::cout << "time_us=" << sim.now() << " frames=" << display.revision() << " pixel=" << display.frame().pixel(2,32) << " LED_mA=" << led.currentAmps()*1000 << '\n';
    return display.revision()==3 && display.frame().pixel(2,32) ? 0 : 1;
}
