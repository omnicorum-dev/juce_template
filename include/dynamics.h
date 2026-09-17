#pragma once

#include "basics.h"
#include "envelope_follower.h"

namespace omni {

/// Downward/upward compressor/expander with soft knee
class Dynamics {
  public:
    /// Which side of the threshold responds, and whether
    /// it compresses (pulls towards threshold) or
    /// expands (pushes away from threshold)
    enum class Mode {
        DOWNWARDS_COMPRESSION, ///< Attenuates signal above threshold
        UPWARDS_COMPRESSION,   ///< Boosts signal below threshold
        DOWNWARDS_EXPANSION,   ///< Attenuates signal below threshold
        UPWARDS_EXPANSION      ///< Boosts signal above threshold
    };

    /// Prepares internal envelope followers and sets their
    /// default ballistics (fast peak detection on input, 50/200ms
    /// attack/release on the gain-reduction smoother)
    /// @param _sample_rate Sample rate in Hz
    /// @param _buffer_size Block size in samples
    void prepare(double _sample_rate, int _buffer_size) {
        fs          = _sample_rate;
        buffer_size = _buffer_size;

        input_envelope_follower.prepare(_sample_rate, _buffer_size);
        gain_envelope_follower.prepare(_sample_rate, _buffer_size);

        gain_envelope_follower.setAttackMs(50);
        gain_envelope_follower.setReleaseMs(200);
        gain_envelope_follower.setMode(EnvelopeFollower::Mode::PEAK);

        input_envelope_follower.setAttackMs(0.1);
        input_envelope_follower.setReleaseMs(0.1);
        input_envelope_follower.setMode(EnvelopeFollower::Mode::PEAK);
    }

    /// Selects Downward/Upward Compression/Expansion
    void setType(Mode _mode) { mode = _mode; }

    /// Selects either peak or RMS envelope following for gain reduction
    void setFollowerMode(EnvelopeFollower::Mode _mode) {
        gain_envelope_follower.setMode(_mode);
    }

    /// Sets attack time of gain-reduction smoother in milliseconds
    void setAttack(double _attack_ms) {
        gain_envelope_follower.setAttackMs(_attack_ms);
    }

    /// Sets release time of gain-reduction smoother in milliseconds
    void setRelease(double _release_ms) {
        gain_envelope_follower.setReleaseMs(_release_ms);
    }

    /// Sets the threshold in dB
    void setThreshold(double _threshold_dB) { threshold_dB = _threshold_dB; }

    /// Sets the compression/expansion ratio (e.g. 4 for 4:1)
    void setRatio(double _ratio) { ratio = _ratio; }

    /// Sets the knee width in dB. 0 -> hard knee
    void setKnee(double _knee_dB) {
        knee_dB      = _knee_dB;
        half_knee_dB = 0.5 * knee_dB;
    }

    /// Processes one sample: computes target gain reduction and
    /// smooths it with envelope follower.
    double processSample(double xn) {
        double target_gr_dB = calculateTargetGain_dB(xn);
        double smoothed_gr_dB =
            gain_envelope_follower.processSample(target_gr_dB);
        double gain = db2mag(-smoothed_gr_dB);
        return xn * gain;
    }

  protected:
    /// Static gain-computer curve: level -> instantaneous (unsmoothed)
    /// target gain reduction in dB, positive = attenuate, negative = boost.
    /// `sign` mirrors the active side (above/below threshold) into a
    /// shared excess `e` so one three-branch (below/in-knee/above) formula
    /// serves all four `Mode`s; `slope` (1/ratio for compression, ratio
    /// for expansion) is applied within that shared formula.
    double calculateTargetGain_dB(double xn) {
        double slope = ratio;
        if (mode == Mode::DOWNWARDS_COMPRESSION ||
            mode == Mode::UPWARDS_COMPRESSION) {
            slope = 1 / ratio;
        }

        int sign = -1;
        if (mode == Mode::DOWNWARDS_COMPRESSION ||
            mode == Mode::UPWARDS_EXPANSION) {
            sign = 1;
        }

        double level    = input_envelope_follower.processSample(xn);
        double level_dB = mag2db(level);

        double e = sign * (level_dB - threshold_dB);
        double f = 0;

        if (e <= -half_knee_dB) {
            f = 0;
        } else if (e <= half_knee_dB) {
            f = (1 - slope) * std::pow(e + half_knee_dB, 2) / (2 * knee_dB);
        } else {
            f = e * (1 - slope);
        }

        return sign * f;
    }

  private:
    double fs          = 48000;
    int    buffer_size = 512;

    EnvelopeFollower input_envelope_follower;
    EnvelopeFollower gain_envelope_follower;

    Mode mode = Mode::DOWNWARDS_COMPRESSION;

    double threshold_dB = 0;
    double ratio        = 1;
    double knee_dB      = 0;
    double half_knee_dB = 0;
};

} // namespace omni
