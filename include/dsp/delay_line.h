#pragma once

#include "dsp/ring_buffer.h"
#include "general/basics.h"
#include "general/value_smoother.h"

/// @file
/// A feedback delay line built on RingBuffer (storage) and
/// ValueSmoother (click-free delay-time changes), with a pluggable
/// feedback-shaping hook for damping/saturation.

namespace omni {

/// Feedback delay line with smoothed delay-time changes.
/// @tparam max_buffer_size Maximum delay length in samples, fixed at
///                          compile time (see RingBuffer).
template <int max_buffer_size> class DelayLine {
  public:
    /// @param _sample_rate Audio sample rate in Hz.
    /// @param _buffer_size Host block size (currently unused).
    void prepare(double _sample_rate, int _buffer_size) {
        fs          = _sample_rate;
        buffer_size = _buffer_size;

        smoother.prepare(fs);
        smoother.setType(SmoothingType::LINEAR);
        smoother.setSmoothingTime(0.02); // 20ms
        smoother.setCurrentAndTargetValue(smoother.getCurrentValue());
    }

    enum class Interpolation { Linear, Hermite, Lagrange3 };

    void setInterpolation(Interpolation i) { interp = i; }

    /// Sets the target delay in milliseconds; the read position glides
    /// there per the current smoothing settings rather than jumping.
    void setDelayTimeMs(double delay_ms) {
        double delay_samples = ms2samples(delay_ms, fs);
        // 1 sample is the shortest delay a feedback loop can have
        delay_samples =
            std::clamp(delay_samples, 1.0, (double)(max_buffer_size - 1));
        smoother.setTargetValue(delay_samples);
    }

    /// Feedback gain, clamped to [-1, 1]
    void setFeedback(double _feedback) {
        feedback = std::clamp(_feedback, -1.0, 1.0);
    }

    /// See ValueSmoother::SmoothingType
    void setSmoothingType(SmoothingType type) { smoother.setType(type); }

    /// See ValueSmoother::setSmoothingTime()
    void setSmoothingTime(double time_sec) {
        smoother.setSmoothingTime(time_sec);
    }

    /// Function applied to the delayed sample before it is returned
    /// AND before it's written back for feedback. Both the output tap
    /// and the feedback path go through this. Use for damping, saturation,
    /// etc. Defaults to identity passthrough.
    void setFeedbackFunction(double (*function)(double)) {
        feedback_function = function;
    }

    /// Reads the delayed (and feedback_function-shaped) sample, then
    /// writes `xn + feedback * delayed` back into the buffer. Read
    /// happens before write, so the minimum achievable delay is 1 sample.
    ///
    /// @param mod_samples Per-sample delay offset in samples (e.g. LFO *
    ///                    depth). Added *after* the smoother, so fast
    ///                    modulation isn't low-passed by the 20 ms ramp.
    ///                    The total is clamped to [1, max_buffer_size - 1].
    double processSample(double xn, double mod_samples = 0.0) {
        // base delay glides via the smoother (knob changes)
        // modulation rides on top of it unsmoothed.
        // The clamp also covers the smoother starting at 0 before
        // the first target arrives.
        const double delay_samples =
            std::clamp(smoother.getValue() + mod_samples, min_delay, max_delay);

        // read() is relative to the last pushed sample, which is already 1
        // sample old, so the read offset is one less than the delay
        const double r = delay_samples - 1.0;

        double delayed;
        if (r < 1.0) {
            // 4-point read needs the sample one step NEWER than floor(r).
            // For r < 1, that would be the current input, which hasn't been
            // pushed yet (and in a feedback loop can't exist), so fall
            // back to linear in this range only.
            delayed = buffer.readFractionalLinear(r);
        } else {
            switch (interp) {
            case Interpolation::Linear:
                delayed = buffer.readFractionalLinear(r);
                break;
            case Interpolation::Lagrange3:
                delayed = buffer.readFractionalLagrange3(r);
                break;
            case Interpolation::Hermite:
            default:
                delayed = buffer.readFractionalHermite(r);
                break;
            }
        }

        delayed = feedback_function(delayed);
        buffer.push(xn + feedback * delayed);
        return delayed;
    }

    /// Empties the buffer and resets the smoother to its current value
    void clear() {
        buffer.clear();
        smoother.setCurrentAndTargetValue(smoother.getCurrentValue());
    }

  private:
    double fs          = 48000;
    int    buffer_size = 512;

    /// Extra buffer samples so 4-point reads near max delay never wrap
    /// onto the newest sample. 1 is the minimum; 4 leaves room for a
    /// 6-point interpolator later.
    static constexpr int    guard     = 4;
    static constexpr double min_delay = 1.0;
    static constexpr double max_delay = (double)(max_buffer_size - 1);

    RingBuffer<max_buffer_size + guard> buffer;

    ValueSmoother smoother;
    Interpolation interp = Interpolation::Hermite;

    double feedback = 0.;

    double (*feedback_function)(double) = [](double xn) { return xn; };
};

} // namespace omni
