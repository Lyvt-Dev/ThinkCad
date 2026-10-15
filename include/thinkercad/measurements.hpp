#pragma once
#include <optional>
#include <vector>
#include <string>
namespace thinkercad {
struct TimedValue {double seconds;std::optional<double> value;};
struct Measurements {double minimum=0,maximum=0,rms=0;std::optional<double> frequency,duty;};
Measurements measure(const std::vector<TimedValue>& samples);
struct DecodedByte {double seconds;unsigned value;bool valid=true;std::string detail;};
std::vector<DecodedByte> decodeUart(const std::vector<TimedValue>& samples,unsigned baud);
std::vector<DecodedByte> decodeSpi(const std::vector<TimedValue>& clock,const std::vector<TimedValue>& data,bool rising=true,bool msbFirst=true);
std::vector<DecodedByte> decodeI2c(const std::vector<TimedValue>& clock,const std::vector<TimedValue>& data);
}
