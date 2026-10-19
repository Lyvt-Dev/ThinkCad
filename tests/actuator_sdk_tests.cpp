#include "Stepper.h"
#include "AccelStepper.h"
#include <stdexcept>
static void check(bool value) {
    if (!value)
        throw std::runtime_error("Actuator SDK contract failed");
}
int main() {
    thinker::standalone = true;
    Stepper full(200, 2, 3, 4, 5);
    full.setSpeed(60);
    const auto before = thinker::clockUs;
    full.step(10);
    check(thinker::clockUs - before == 50000);
    full.step(-10);
    AccelStepper driver(AccelStepper::DRIVER, 12, 13);
    driver.setMaxSpeed(100);
    driver.setAcceleration(100);
    driver.moveTo(20);
    int guard = 0;
    while (driver.run() && guard++ < 10000)
        delayMicroseconds(1000);
    check(guard < 10000 && driver.currentPosition() == 20 && driver.distanceToGo() == 0);
    driver.move(-10);
    guard = 0;
    while (driver.run() && guard++ < 10000)
        delayMicroseconds(1000);
    check(driver.currentPosition() == 10);
    driver.disableOutputs();
    driver.setSpeed(100);
    delayMicroseconds(100000);
    check(!driver.runSpeed() && driver.currentPosition() == 10);
}
