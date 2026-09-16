#pragma once

#include "basics.h"
#include "ring_buffer.h"
#include "value_smoother.h"

namespace omni {

template <int max_buffer_size> class DelayLine {
  public:
    void prepare(double _sample_rate, int _buffer_size) {
        fs          = _sample_rate;
        buffer_size = _buffer_size;

        smoother.prepare(fs);
        smoother.setType(SmoothingType::LINEAR);
        smoother.setSmoothingTime(0.02); // 20ms
        smoother.setCurrentAndTargetValue(smoother.getCurrentValue());
    }

    void setDelayTimeMs(double delay_ms) {
        double delay_samples = ms2samples(delay_ms, fs);
        delay_samples =
            std::clamp(delay_samples, 0., (double)(max_buffer_size - 2));
        smoother.setTargetValue(delay_samples);
    }

    void setFeedback(double _feedback) {
        feedback = std::clamp(_feedback, -1.0, 1.0);
    }

    void setSmoothingType(SmoothingType type) { smoother.setType(type); }

    void setSmoothingTime(double time_sec) {
        smoother.setSmoothingTime(time_sec);
    }

    void setFeedbackFunction(double (*function)(double)) {
        feedback_function = function;
    }

    double processSample(double xn) {
        double delay_samples = smoother.getValue();
        double delayed       = buffer.readFractional_linear(delay_samples);

        delayed = feedback_function(delayed);

        buffer.push(xn + feedback * delayed);

        return delayed;
    }

    void clear() {
        buffer.clear();
        smoother.setCurrentAndTargetValue(smoother.getCurrentValue());
    }

  private:
    double fs          = 48000;
    int    buffer_size = 512;

    RingBuffer<max_buffer_size> buffer;
    ValueSmoother               smoother;

    double feedback = 0.;

    double (*feedback_function)(double) = [](double xn) { return xn; };
};

} // namespace omni
