#pragma once

#include <cmath>

namespace omni {

enum class Polarity {
    UNIPOLAR,
    BIPOLAR,
};

class BasicOscillator {
  public:
    void prepare(double _sample_rate, int _buffer_size) {
        fs          = _sample_rate;
        buffer_size = _buffer_size;
        reset();
    }

    // The generator function receives phase in the range [0, 1)
    // (defaults to a sine wave)
    void setGeneratorFunction(double (*function)(double)) {
        generatorFunction = function;
    }

    void reset() {
        if (fs > 0.f) {
            phase_increment = frequency / fs;
        }
        phase = 0;
    }

    void setFrequency(double hz) {
        frequency = hz;
        if (fs > 0)
            phase_increment = frequency / fs;
    }

    void setPolarity(Polarity _polarity) { polarity = _polarity; }

    void setPhaseOffset(double offset) { phase_offset = offset; }

    double processSample(double phase_mod = 0.0, double frequency_mod = 0.0) {
        double sample = generateSample(phase + phase_mod);

        phase += frequency_mod == 0.0 ? phase_increment
                                      : (frequency + frequency_mod) / fs;

        phase -= std::floor(phase);

        return applyPolarity(sample);
    }

    double getPhase() { return phase; }
    double getFreqyency() { return frequency; }

  protected:
    double generateSample(double p) const {
        double ph = std::fmod(p + phase_offset, 1.0);
        if (ph < 0.0)
            ph += 1.0;

        return generatorFunction(ph);
    }

    double applyPolarity(double sample) const {
        if (polarity == Polarity::UNIPOLAR)
            return (sample + 1.0) * 0.5;
        return sample;
    }

    void advancePhase() {
        phase += phase_increment;
        if (phase >= 1.0)
            phase -= 1.0;
    }

  private:
    double phase_increment = 0;
    double phase           = 0;
    double phase_offset    = 0;
    double frequency       = 10;

    Polarity polarity = Polarity::BIPOLAR;

    double (*generatorFunction)(double) = [](double ph) {
        return std::sin(ph * 2 * M_PI);
    };

    double fs          = 0.f;
    int    buffer_size = 512;
};

} // namespace omni
