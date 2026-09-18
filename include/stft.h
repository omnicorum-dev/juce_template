#pragma once

#include <array>
#include <cassert>
#include <complex>
#include <cstddef>
#include <initializer_list>
#include <vector>

#include "basics.h"
#include "fft.h"
#include "fifo.h"
#include "overlap_add.h"
#include "ring_buffer.h"

namespace omni {

#define SpectralFn [&](std::complex<double> * spectrum, size_t num_bins)

template <size_t max_size> class STFT {
  public:
    static constexpr size_t max_spectrum_size = max_size / 2 + 1;

    STFT(std::initializer_list<size_t> sizes) {
        slots.reserve(sizes.size());
        for (size_t size : sizes) {
            slots.emplace_back(size);
        }
        setSize(*sizes.begin());
    }

    void setSize(size_t size) {
        active = findSlot(size);
        history.clear();
        overlap_add.clear();
        wet_queue.clear();
        hop_counter = 0;
    }

    size_t getLatencySamples() const { return active->hop_size; }

    template <typename SpectralFunction>
    double processSample(double input, SpectralFunction &&spectral_fn) {
        history.push(input);

        if (++hop_counter >= (int)active->hop_size) {
            hop_counter = 0;
            processFrame(spectral_fn);
        }

        return wet_queue.pop();
    }

  private:
    struct Slot {
        FFT                 fft;
        std::vector<double> window;
        size_t              size;
        size_t              hop_size;
        size_t              spectrum_size;

        Slot(size_t s)
            : fft(s), window(makeSqrtHannWindow(s)), size(s), hop_size(s / 2),
              spectrum_size(s / 2 + 1) {}
    };

  private:
    Slot *findSlot(size_t size) {
        for (auto &s : slots) {
            if (s.size == size)
                return &s;
        }
        assert(false && "STFT: Unsupported size requested");
        return &slots.front();
    }

    template <typename SpectralFunction>
    void processFrame(SpectralFunction &&spectral_fn) {
        if (history.getSize() < (int)active->size)
            return;

        history.getRecent(fft_input.data(), (int)active->size);

        for (size_t i = 0; i < active->size; ++i) {
            fft_input[i] *= active->window[i];
        }

        active->fft.forward(fft_input.data(), spectrum.data());

        spectral_fn(spectrum.data(), active->spectrum_size);

        active->fft.inverse(spectrum.data(), fft_output.data());

        for (size_t i = 0; i < active->size; ++i) {
            fft_output[i] *= active->window[i];
        }

        std::array<double, max_size> hop_out{};

        overlap_add.addFrameAndAdvance(
            fft_output.data(), active->size, active->hop_size, hop_out.data());

        wet_queue.push(hop_out.data(), active->hop_size);
    }

  private:
    std::vector<Slot> slots;
    Slot             *active = nullptr;

    RingBuffer<max_size>                                history{};
    std::array<double, max_size>                        fft_input{};
    std::array<std::complex<double>, max_spectrum_size> spectrum{};
    std::array<double, max_size>                        fft_output{};

    OverlapAddBuffer<max_size> overlap_add{};
    Fifo<max_size * 2>         wet_queue{};
    int                        hop_counter = 0;
};

} // namespace omni
