#pragma once
#include "thinkercad/circuit.hpp"
#include <deque>
namespace thinkercad::measurement {
struct Sample {
    Microseconds time = 0;
    std::vector<std::optional<double>> channels;
};
class TraceStore {
  public:
    explicit TraceStore(std::size_t capacity = 20000);
    void append(Sample sample);
    void clear() {
        data_.clear();
        overwritten_ = 0;
    }
    const std::deque<Sample> &samples() const {
        return data_;
    }
    std::uint64_t overwritten() const {
        return overwritten_;
    }
    std::optional<double> valueAt(Microseconds time, std::size_t channel) const;

  private:
    std::size_t capacity_;
    std::uint64_t overwritten_ = 0;
    std::deque<Sample> data_;
};
Sample acquire(const CircuitPart &, Microseconds, const CircuitState &);
bool crossed(const Sample &previous, const Sample &next, std::size_t channel, double threshold, bool rising);
std::string csv(const std::deque<Sample> &, const std::vector<std::string> &columns);
} // namespace thinkercad::measurement
