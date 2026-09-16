#pragma once

#include <array>
#include <cstddef>

namespace omni {

template <int max_buffer_size> class RingBuffer {
  public:
    void push(double xn) {
        buffer[write_head] = xn;
        write_head         = wrap(write_head + 1);
        if (size < capacity)
            ++size;
    }

    void push(const double *data, int num_samples) {
        for (int i = 0; i < num_samples; ++i)
            push(data[i]);
    }

    double read(int delay_samples = 0) const {
        int index = wrap(write_head - 1 - delay_samples);
        return buffer[index];
    }

    double readFractional_linear(double delay_samples) {
        int    d0   = (int)delay_samples;
        double frac = delay_samples - d0;
        double s0   = read(wrap(d0));
        double s1   = read(wrap(d0 + 1));
        return s0 + frac * (s1 - s0);
    }

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
