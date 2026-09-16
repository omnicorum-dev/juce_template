#pragma once

#include <algorithm>

namespace omni {

class EnvelopeFollower {
  public:
    enum class Mode { PEAK, RMS };

    void prepare(double _sample_rate, int _buffer_size) {
        fs          = _sample_rate;
        buffer_size = _buffer_size;
        updateCoeffs();
    }

    void setMode(Mode _mode) { mode = _mode; }

    void setAttackMs(double ms) {
        attack_ms = ms;
        updateCoeffs();
    }

    void setReleaseMs(double ms) {
        release_ms = ms;
        updateCoeffs();
    }

    double processSample(double xn) {
        return mode == Mode::PEAK ? processPeak(xn) : processRMS(xn);
    }

    void reset() {
        envelope    = 0;
        envelope_sq = 0;
    }

  private:
    void updateCoeffs() {
        attack_coeff  = std::exp(-1. / (attack_ms * 0.001 * fs));
        release_coeff = std::exp(-1. / (release_ms * 0.001 * fs));
    }

    double processPeak(double xn) {
        double rectified = std::abs(xn);
        double coeff     = rectified > envelope ? attack_coeff : release_coeff;
        envelope         = rectified + coeff * (envelope - rectified);
        return envelope;
    }

    double processRMS(double xn) {
        double squared = xn * xn;
        double coeff   = squared > envelope_sq ? attack_coeff : release_coeff;
        envelope_sq    = squared + coeff * (envelope_sq - squared);
        return std::sqrt(std::max(envelope_sq, 0.));
    }

  private:
    double fs          = 48000;
    int    buffer_size = 512;

    Mode mode = Mode::PEAK;

    double attack_ms  = 10.;
    double release_ms = 100.;

    double attack_coeff  = 0.;
    double release_coeff = 0.;

    double envelope    = 0.;
    double envelope_sq = 0.;
};

} // namespace omni
