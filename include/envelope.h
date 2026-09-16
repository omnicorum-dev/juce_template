#pragma once

#include <algorithm>
#include <cmath>

class AHDSR {
  public:
    enum class Stage { Idle, Attack, Hold, Decay, Sustain, Release };
    enum class RampShape { Linear, Exponential };

    void prepare(double _sample_rate, int _buffer_size) {
        sample_rate = _sample_rate;
        buffer_size = _buffer_size;
    }

    void setRampShape(RampShape shape) { ramp_shape = shape; }

    void setParameters(double attack_s, double hold_s, double decay_s,
                       double sustain_amt, double release_s) {
        attack_time   = std::max(attack_s, 0.0001);
        hold_time     = std::max(hold_s, 0.0);
        decay_time    = std::max(decay_s, 0.0001);
        sustain_level = std::clamp(sustain_amt, 0.0, 1.0);
        release_time  = std::max(release_s, 0.0001);
    }

    void noteOn() {
        stage = Stage::Attack;
        // ramp from wherever we are instead of jumping to 0 before ramping
        attack_start_level = current_level;
    }

    void noteOff() {
        if (stage != Stage::Idle) {
            stage               = Stage::Release;
            release_start_level = current_level;
        }
    }

    bool isActive() const { return stage != Stage::Idle; }

    // updates envelope and returns the current envelope level (0.0 - 1.0)
    double update() {
        switch (stage) {
        case Stage::Idle:
            current_level = 0.0;
            break;
        case Stage::Attack:
            current_level = advanceTo(
                current_level, 1.0, attack_time, attack_start_level, 1.0);
            if (reachedTarget(current_level, 1.0, true)) {
                current_level          = 1.0;
                hold_samples_remaining = (long long)(hold_time * sample_rate);
                stage =
                    (hold_samples_remaining > 0) ? Stage::Hold : Stage::Decay;
            }
            break;
        case Stage::Hold:
            current_level = 1.0;
            if (--hold_samples_remaining <= 0) {
                stage = Stage::Decay;
            }
            break;
        case Stage::Decay:
            current_level = advanceTo(
                current_level, sustain_level, decay_time, 1.0, sustain_level);
            if (reachedTarget(current_level, sustain_level, false)) {
                current_level = sustain_level;
                stage         = Stage::Sustain;
            }
            break;
        case Stage::Sustain:
            current_level = sustain_level;
            break;
        case Stage::Release:
            current_level = advanceTo(
                current_level, 0.0, release_time, release_start_level, 0.0);
            if (reachedTarget(current_level, 0.0, false)) {
                current_level = 0.0;
                stage         = Stage::Idle;
            }
            break;
        };

        return current_level;
    }

  private:
    double advanceTo(double current, double target, double time,
                     double start_level, double end_level) {
        switch (ramp_shape) {
        case RampShape::Linear: {
            double span = std::abs(end_level - start_level);
            if (span < 1e-9)
                return target;
            double increment = span / (time * sample_rate);
            return current + (target > current ? increment : -increment);
        }
        case RampShape::Exponential: {
            double coeff = std::exp(std::log(0.01) / (time * sample_rate));
            return target + (current - target) * coeff;
        }
        default:
            return target;
        };
    }

    bool reachedTarget(double current, double target, bool rising) const {
        constexpr double epsilon = 0.0005;
        return rising ? (current >= target - epsilon)
                      : (current <= target + epsilon);
    }

  private:
    double sample_rate = 48000;
    int    buffer_size = 512;

    double attack_time   = 0.01;
    double hold_time     = 0.0;
    double decay_time    = 0.1;
    double sustain_level = 0.7;
    double release_time  = 0.2;

    double current_level = 0.0;

    double attack_start_level  = 0.0;
    double release_start_level = 0.0;

    long long hold_samples_remaining = 0;

    RampShape ramp_shape = RampShape::Linear;
    Stage     stage      = Stage::Idle;
};
