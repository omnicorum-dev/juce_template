#pragma once

#include <algorithm>
#include <cstddef>

namespace omni {

template <size_t maxSize> class OverlapAddBuffer {
  public:
    void clear() { buffer.fill(0.0); }

    void addFrameAndAdvance(const double *frame, size_t frameSize,
                            size_t hopSize, double *output) {
        for (size_t i = 0; i < frameSize; ++i)
            buffer[i] += frame[i];

        std::copy(buffer.begin(), buffer.begin() + hopSize, output);

        std::copy(buffer.begin() + hopSize,
                  buffer.begin() + frameSize,
                  buffer.begin());

        std::fill(buffer.begin() + (frameSize - hopSize),
                  buffer.begin() + frameSize,
                  0.0);
    }

  private:
    std::array<double, maxSize> buffer{};
};

} // namespace omni
