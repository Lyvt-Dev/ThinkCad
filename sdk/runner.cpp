#include "Arduino.h"
#ifndef THINKERCAD_SKETCH
#error Define THINKERCAD_SKETCH as the absolute sketch filename
#endif
#include THINKERCAD_SKETCH
int main(int argc, char** argv) {
    try {
        unsigned long iterations = 0;
        if (argc == 3 && std::string(argv[1]) == "--iterations") iterations = std::stoul(argv[2]);
        thinker::standalone = iterations != 0;
        setup();
        std::cout << "READY" << std::endl;
        for (unsigned long i=0; iterations == 0 || i<iterations; ++i) { loop(); yield(); }
        return 0;
    } catch (const std::exception& e) { std::cerr << "Sketch failed: " << e.what() << '\n'; return 1; }
}
