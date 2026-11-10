#pragma once

#include "dsp/ring_buffer.h"
#include "general/value_smoother.h"
#include <array>
#include <cmath>

namespace omni {

enum class MixLaw { Linear, EqualPower };

/// Latency-compensated dry path + wet/dry mix + click-free bypass.
/// Call `tick()` once per sample (outside channel loop), then
/// `dry()`, `mix()`, and `bypass()` per channel.
template <int max_latency, size_t max_channels = 2> class DryWet {
  public:
    void prepare(double fs, int latency_samples, bool start_bypassed) {
        latency = std::clamp(latency_samples, 0, max_latency);
        for (auto &d : dry_delay) {
            d.clear();
        }

        bypass_fade.prepare(fs);
        bypass_fade.setType(SmoothingType::LINEAR);
        bypass_fade.setSmoothingTime(0.01); // 10 ms
        // no fade on load: jump straight to saved state
        bypass_fade.setCurrentAndTargetValue(start_bypassed ? 1.0 : 0.0);
    }

    void setMixLaw(MixLaw l) { law = l; }
    void setBypassed(bool b) { bypass_fade.setTargetValue(b ? 1.0 : 0.0); }

    int getLatency() const { return latency; }

    /// Once per sample. mix in [0, 1]
    void tick(double mix) {
        switch (law) {
        case MixLaw::EqualPower:
            wet_gain = std::sin(mix * M_PI * 0.5);
            dry_gain = std::cos(mix * M_PI * 0.5);
            break;
        case MixLaw::Linear:
        default:
            wet_gain = mix;
            dry_gain = 1.0 - mix;
        }
        bypass_amt = bypass_fade.getValue();
    }

    /// Push raw input, get it back `latency` samples later
    double dry(size_t ch, double xn) {
        dry_delay[ch].push(xn);
        return dry_delay[ch].read(latency);
    }

    double mix(double wet, double dry_delayed) const {
        return wet * wet_gain + dry_delayed * dry_gain;
    }

    /// Final stage: crossfade fully-processed output to pure dry
    /// to prevent clicks
    double bypass(double yn, double dry_delayed) const {
        return yn + (dry_delayed - yn) * bypass_amt;
    }

  private:
    std::array<RingBuffer<max_latency + 1>, max_channels> dry_delay{};

    ValueSmoother bypass_fade;
    MixLaw        law      = MixLaw::Linear;
    int           latency  = 0;
    double        wet_gain = 1, dry_gain = 0, bypass_amt = 0;
};

} // namespace omni
