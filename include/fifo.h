#pragma once

#include <array>
#include <cstddef>

namespace omni {

template <size_t capacity> class Fifo {
  public:
    void clear() {
        buffer.fill(0.0);
        write_index = read_index = count = 0;
    }

    void push(const double value) {
        buffer[write_index] = value;
        write_index         = (write_index + 1) % capacity;
        if (count < capacity)
            ++count;
    }

    void push(const double *data, size_t n) {
        for (size_t i = 0; i < n; ++i) {
            push(data[i]);
        }
    }

    double pop() {
        if (count == 0)
            return 0.0;
        double v   = buffer[read_index];
        read_index = (read_index + 1) % capacity;
        --count;
        return v;
    }

  private:
    std::array<double, capacity> buffer{};

    size_t write_index = 0, read_index = 0, count = 0;
};

} // namespace omni
