#pragma once

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
        buffer[(size_t)write_head] = xn;
        write_head                 = wrap(write_head + 1);
        if (size < capacity)
            ++size;
    }

    /// Writes `num_samples` samples in order
    void push(const double *data, int num_samples) {
        for (int i = 0; i < num_samples; ++i)
            push(data[i]);
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
    double readFractional_linear(double delay_samples) {
        int    d0   = (int)delay_samples;
        double frac = delay_samples - d0;
        double s0   = read(wrap(d0));
        double s1   = read(wrap(d0 + 1));
        return s0 + frac * (s1 - s0);
    }

    void getRecent(double *output, int num_samples) const {
        assert(num_samples <= size);

        const int start = wrap(write_head - num_samples);

        const int first = std::min(num_samples, capacity - start);

        std::copy(
            buffer.begin() + start, buffer.begin() + start + first, output);

        if (first < num_samples) {
            std::copy(buffer.begin(),
                      buffer.begin() + (num_samples - first),
                      output + first);
        }
    }

    /// Zeros the buffer and resets read/write position
    void clear() {
        write_head = 0;
        size       = 0;
        buffer.fill(0);
    }

    int           getCapacity() const { return capacity; }
    int           getSize() const { return size; }
    bool          isEmpty() const { return size == 0; }
    bool          isFull() const { return size == capacity; }
    const double *getData() const { return buffer.data(); }
    int           getWriteHead() const { return write_head; }

  protected:
    int wrap(int index) const {
        return ((index % capacity) + capacity) % capacity;
    }

  private:
    std::array<double, max_buffer_size> buffer{};

    int capacity   = max_buffer_size;
    int write_head = 0;
    int size       = 0;
};

} // namespace omni
