#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>

/// @file
/// Fixed-capacity circular sample buffer with delay-style reads
/// (including linear-interpolated fractional reads) -- the storage
/// primitive behind DelayLine.

namespace omni {

/// Fixed-capacity circular buffer of `double` samples, indexed by delay
/// (samples ago) rather than by absolute position.
/// @tparam max_buffer_size Capacity in samples, fixed at compile time.
template <int max_buffer_size> class RingBuffer {
  public:
    /// Writes one sample, overwriting the oldest once full
    void push(double xn) {
        buffer[write_head]            = xn;
        buffer[write_head + capacity] = xn;

        write_head = wrap(write_head + 1);
        if (size < capacity) {
            ++size;
        }
    }

    /// Writes `num_samples` samples in order
    void push(const double *data, size_t num_samples) {
        for (size_t i = 0; i < num_samples; ++i) {
            push(data[i]);
        }
    }

    /// Reads the sample `delay_samples` ago (0 = most recently pushed)
    double read(int delay_samples = 0) const {
        int index = wrap(write_head - 1 - delay_samples);
        return buffer[index];
    }

    /// Linearly-interpolated read at a fractional delay in samples
    /// @note the wrap() calls on d0/d0+1 here are redundant with the
    ///       wrapping read() already does internally -- harmless since
    ///       wrap() is idempotent, but worth knowing if you're tracing
    ///       through this.
    double readFractionalLinear(double delay_samples) const {
        int    d0   = (int)delay_samples;
        double frac = delay_samples - d0;
        double s0   = read(d0);
        double s1   = read(d0 + 1);
        return s0 + frac * (s1 - s0);
    }

    /// 4-point, 3rd-order Hermite (Catmull-Rom)
    /// Smooth first derivative, so modulated reads stay smooth.
    /// @pre 1 <= delay < capacity - 2
    double readFractionalHermite(double delay_samples) const {
        const int     d   = (int)delay_samples;
        const double  t   = delay_samples - d;
        const double *p   = taps4(d);
        const double  xm1 = p[3], x0 = p[2], x1 = p[1], x2 = p[0];

        const double c1 = 0.5 * (x1 - xm1);
        const double c2 = xm1 - 2.5 * x0 + 2.0 * x1 - 0.5 * x2;
        const double c3 = 0.5 * (x2 - xm1) + 1.5 * (x0 - x1);
        return ((c3 * t + c2) * t + c1) * t + x0;
    }

    /// 4-point, 3rd-order Lagrange. Maximally flat at DC.
    /// @pre 1 <= delay < capacity - 2
    double readFractionalLagrange3(double delay_samples) const {
        const int     d   = (int)delay_samples;
        const double  t   = delay_samples - d;
        const double *p   = taps4(d);
        const double  xm1 = p[3], x0 = p[2], x1 = p[1], x2 = p[0];

        const double tm1 = t - 1.0, tm2 = t - 2.0, tp1 = t + 1.0;
        return -xm1 * t * tm1 * tm2 / 6.0 + x0 * tp1 * tm1 * tm2 / 2.0 -
               x1 * tp1 * t * tm2 / 2.0 + x2 * tp1 * t * tm1 / 6.0;
    }

    /// Copies the most recently pushed samples to an output buffer
    /// Samples are copied in chronological order, from oldest to
    /// newest.
    /// @param output Destination buffer for the samples
    /// @param num_samples Number of recent samples to retrieve
    ///
    /// @note @p num_samples must not exceed the current number of samples
    ///       stored in the buffer
    void getRecent(double *output, size_t num_samples) const {
        assert(num_samples <= size);
        const int start = wrap(static_cast<int>(write_head - num_samples));
        std::copy(buffer.begin() + start,
                  buffer.begin() + start + num_samples,
                  output);
    }

    /// 4 contiguous samples, oldest first: delays d+2, d+1, d, d-1.
    /// Uses mirrored upper half of buffer to prevent per-tap wrapping.
    /// @pre 1 <= d <= capacity - 3
    const double *taps4(int d) const {
        return &buffer[wrap(write_head - 1 - (d + 2))];
    }

    /// Zeros the buffer and resets read/write position
    void clear() {
        write_head = 0;
        size       = 0;
        buffer.fill(0);
    }

    size_t        getCapacity() const { return capacity; }
    size_t        getSize() const { return size; }
    bool          isEmpty() const { return size == 0; }
    bool          isFull() const { return size == capacity; }
    const double *getData() const { return buffer.data(); }
    size_t        getWriteHead() const { return write_head; }

  protected:
    int wrap(int index) const {
        const int cap = static_cast<int>(capacity);
        return ((index % cap) + cap) % cap;
    }

  private:
    static_assert(max_buffer_size > 0, "rb2: capacity must be positive");

    std::array<double, max_buffer_size * 2> buffer{};

    int capacity   = max_buffer_size; // was size_t — kept as int so `wrap`'s
                                      // signed-modulo trick works correctly
    int write_head = 0; // for negative inputs (e.g. delay > write_head)
    int size       = 0;
};

} // namespace omni
