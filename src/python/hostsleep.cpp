#include "../../sdk/hostsleep.hpp"
#include <chrono>
#include <cstdint>
extern "C" void thinkcad_sleep_us(std::uint64_t us) {
    thinker::hostSleep(std::chrono::microseconds(us));
}
