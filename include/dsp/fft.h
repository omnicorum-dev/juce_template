#pragma once

#include <cassert>
#include <pffft/pffft.h>
#include <pffft/pffft_double.h>

#include <complex>
#include <vector>

/// @file
/// PFFFT wrapper, allowing for easy FFT and IFFT

namespace omni {

/// std::allocator replacement that returns `Align`-byte-aligned memory
template <typename T, std::size_t Align = 32> struct AlignedAllocator {
    using value_type = T;

    // Needed because of the non-type template parameter
    template <typename U> struct rebind {
        using other = AlignedAllocator<U, Align>;
    };

    AlignedAllocator() noexcept = default;
    template <typename U>
    AlignedAllocator(const AlignedAllocator<U, Align> &) noexcept {}

    T *allocate(std::size_t n) {
        return static_cast<T *>(
            ::operator new(n * sizeof(T), std::align_val_t{Align}));
    }
    void deallocate(T *p, std::size_t) noexcept {
        ::operator delete(p, std::align_val_t{Align});
    }

    template <typename U>
    bool operator==(const AlignedAllocator<U, Align> &) const noexcept {
        return true;
    }
    template <typename U>
    bool operator!=(const AlignedAllocator<U, Align> &) const noexcept {
        return false;
    }
};

template <typename T> using AlignedVector = std::vector<T, AlignedAllocator<T>>;

/// PFFFT wants 32-byte-aligned pointers on x86, 16 on ARM
inline bool isSimdAligned(const void *p) {
    return (reinterpret_cast<std::uintptr_t>(p) & 31) == 0;
}

/// FFT and IFFT wrapper. Does not hold any input/output state
class FFT {
  public:
    /// Must declare FFT size at construction of object
    FFT(size_t size) : _size(size), work(size), buffer(size) {
        assert(size != 0 && "FFT size cannot be zero");
        setup = pffftd_new_setup((int)size, PFFFT_REAL);
        assert(setup && "Failed to create PFFFT setup");
    }

    ~FFT() {
        if (setup) {
            pffftd_destroy_setup(setup);
        }
    }

    FFT(const FFT &)            = delete;
    FFT &operator=(const FFT &) = delete;

    FFT(FFT &&other) noexcept
        : _size(other._size), setup(other.setup), work(std::move(other.work)),
          buffer(std::move(other.buffer)) {
        other.setup = nullptr;
    }

    FFT &operator=(FFT &&other) noexcept {
        if (this != &other) {
            if (setup)
                pffftd_destroy_setup(setup);
            _size       = other._size;
            setup       = other.setup;
            work        = std::move(other.work);
            buffer      = std::move(other.buffer);
            other.setup = nullptr;
        }
        return *this;
    }

    /// Forward FFT. Uses REAL inputs (time) and produces COMPLEX outputs
    /// (spectrum)
    void forward(const double *input, std::complex<double> *output) {
        assert(isSimdAligned(input) && "FFT input must be 32-byte aligned");

        pffftd_transform_ordered(
            setup, input, buffer.data(), work.data(), PFFFT_FORWARD);

        const size_t half = _size / 2;

        output[0] = {buffer[0], 0};

        for (size_t k = 1; k < half; ++k) {
            output[k] = {buffer[2 * k], buffer[2 * k + 1]};
        }

        output[half] = {buffer[1], 0};
    }

    /// Inverse FFT. Uses COMPLEX inputs (spectrum) and produces REAL
    /// outputs (time)
    void inverse(const std::complex<double> *input, double *output) {
        assert(isSimdAligned(output) && "FFT output must be 32-byte aligned");

        const size_t half = _size / 2;

        buffer[0] = input[0].real();
        buffer[1] = input[half].real();

        for (size_t k = 1; k < half; ++k) {
            buffer[2 * k]     = input[k].real();
            buffer[2 * k + 1] = input[k].imag();
        }

        pffftd_transform_ordered(
            setup, buffer.data(), output, work.data(), PFFFT_BACKWARD);

        const double scale = 1 / (double)_size;

        for (size_t i = 0; i < _size; ++i) {
            output[i] *= scale;
        }
    }

    /// Get the size of the FFT (temporal samples)
    size_t size() const { return _size; }

    /// Get the size of the spectrum output (number of bins)
    size_t spectrumSize() const { return _size / 2 + 1; }

  private:
    std::size_t _size;

    PFFFTD_Setup *setup = nullptr;

    AlignedVector<double> work;
    AlignedVector<double> buffer;
};

} // namespace omni
