#include "thinkercad/simulation.hpp"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace thinkercad;
int checks = 0;
void check(bool ok, const char* what) { ++checks; if (!ok) throw std::runtime_error(what); }
template<class F> void rejects(F action, const char* what) { bool rejected=false; try { action(); } catch (const std::exception&) { rejected=true; } check(rejected,what); }
int main() {
 try {
    Esp8266Board board;
    check(board.digitalRead(5)==Level::Floating,"reset input floats");
    board.pinMode(5,PinMode::InputPullup);
    check(board.digitalRead(5)==Level::High,"pullup");
    board.driveInput(5,Level::Low);
    check(board.digitalRead(5)==Level::Low,"strong driver beats pullup");
    board.pinMode(5,PinMode::Output); board.digitalWrite(5,true);
    check(board.digitalRead(5)==Level::Contention,"conflicting outputs");
    rejects([&]{ board.pinMode(6,PinMode::Output); },"flash pin must reject");
    rejects([&]{ board.pinMode(16,PinMode::InputPullup); },"GPIO16 pullup must reject");
    rejects([&]{ board.digitalWrite(4,true); },"write to input must reject");
    board.setAdcVoltage(0.5); check(board.analogRead()==512,"ADC quantization");
    rejects([&]{ board.setAdcVoltage(3.3); },"raw ADC overvoltage must reject");
    rejects([&]{ board.setAdcVoltage(std::numeric_limits<double>::quiet_NaN()); },"NaN ADC");
    Led led("led"); led.apply(3.3,0);
    check(std::abs(led.currentAmps()-1.3/220)<1e-9,"LED current with series resistor");
    led.apply(0,3.3); check(led.currentAmps()==0,"reverse bias LED");
    rejects([]{ Led bad("bad",0); },"zero resistor");
    OledDisplay display; Frame f; f.setPixel(127,63,true); display.present(f);
    check(display.frame().pixel(127,63) && display.revision()==1,"display update");
    rejects([&]{ display.present(Frame(16,2)); },"invalid display dimensions");
    check(OledDisplay::transferDuration(1026,400000)==23085,"I2C frame timing");
    check(OledDisplay::transferDuration(1026,100000)==92340,"100kHz frame timing");
    rejects([]{ OledDisplay::transferDuration(1026,0); },"zero bus speed");
    Simulator sim; std::vector<int> order;
    sim.schedule(10,[&]{ order.push_back(1); sim.schedule(10,[&]{order.push_back(3);}); });
    sim.schedule(10,[&]{order.push_back(2);}); sim.runUntil(10);
    check(order==std::vector<int>({1,2,3}),"deterministic order including nested events");
    rejects([&]{sim.schedule(9,[]{});},"past event");
    rejects([&]{sim.runUntil(9);},"backwards time");
    sim.reset(); std::function<void()> loop; loop=[&]{sim.schedule(sim.now(),loop);}; sim.schedule(0,loop);
    rejects([&]{sim.runUntil(0,10);},"zero-time event budget");
    sim.reset(); display.reset();
    { Animation a; Frame a0; a0.setPixel(1,1,true); Frame a1; a1.setPixel(2,2,true); a.addFrame(a0,100); a.addFrame(a1,100); a.enqueue(sim,display,0); }
    sim.runUntil(99); check(display.frame().pixel(1,1) && display.revision()==1,"first animation frame retained");
    sim.runUntil(100); check(display.frame().pixel(2,2) && display.revision()==2,"frame boundary");
    rejects([]{ Animation a; a.addFrame(Frame{},0); },"zero duration");
    rejects([&]{ Animation a; a.addFrame(Frame{},10); a.enqueue(sim,display,std::numeric_limits<Microseconds>::max()-5); },"animation overflow");
    board.reset(); check(board.analogRead()==0 && board.digitalRead(5)==Level::Floating,"board reset");
    std::cout << checks << " checks passed\n";
    return 0;
 } catch(const std::exception& e) {std::cerr<<e.what()<<'\n'; return 1;}
}
