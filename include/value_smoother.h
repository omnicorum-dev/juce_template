#pragma once

#include <algorithm>
namespace omni {

enum class SmoothingType { LINEAR, EXPONENTIAL };

class ValueSmoother {
  public:
    void prepare(double _sample_rate) { fs = _sample_rate; }

    void setType(SmoothingType _type) { type = _type; }

    void setSmoothingTime(double time_seconds) {
        ramp_samples = std::max(1.0, time_seconds * fs);
        b1           = std::exp(-1.0 / ramp_samples);
    }

    void setCurrentAndTargetValue(double value) {
        current = target  = value;
        increment         = 0.;
        samples_remaining = 0;
    }

    void setTargetValue(double value) {
        if (value == target)
            return;
        target = value;

        if (type == SmoothingType::LINEAR) {
            samples_remaining = (int)ramp_samples;
            increment         = (target - current) / ramp_samples;
        }
    }

    // progresses the smoothing.
    // use this on 90% of the time.
    double getValue() {
        if (type == SmoothingType::LINEAR) {
            if (samples_remaining <= 0)
                return current;
            current += increment;
            if (--samples_remaining == 0)
                current = target;
        } else {
            current = target + (current - target) * b1;
            if (std::abs(target - current) < settle_threshold)
                current = target;
        }
        return current;
    }

    // does not progress the smoothing.
    double getCurrentValue() const { return current; }

    bool isSmoothing() const {
        return type == SmoothingType::LINEAR
                   ? samples_remaining > 0
                   : std::abs(target - current) >= settle_threshold;
    }

  private:
    double        fs   = 48000;
    SmoothingType type = SmoothingType::LINEAR;

    double current = 0;
    double target  = 0;

    // LINEAR state
    double ramp_samples      = 1;
    double increment         = 0;
    int    samples_remaining = 0;

    // EXPONENTIAL state
    double b1 = 0;

    static constexpr double settle_threshold = 1e-5;
};

} // namespace omni
