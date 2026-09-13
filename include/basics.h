#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace omni {

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef M_2_PI
#define M_2_PI 0.636619772367581343076
#endif

// Conversions

inline double db2mag(double db) { return std::pow(10, db / 20); }

inline double mag2db(double mag) { return 20 * std::log10(mag); }

inline double db2pow(double db) { return std::pow(10, db / 10); }

inline double pow2db(double pow) { return 10 * std::log10(pow); }

inline double bw2q(double f0, double bw) { return f0 / bw; }

inline double q2bw(double f0, double q) { return f0 / q; }

inline double samples2ms(double samples, double sample_rate) {
    return (samples / sample_rate) * 1000;
}

inline double ms2samples(double ms, double sample_rate) {
    return (ms / 1000) * sample_rate;
}

// Useful functions

inline int sign(double x) {
    if (x > 0)
        return 1;
    if (x < 0)
        return -1;
    return 0;
}

inline double rms(const double *data, int num_samples) {
    double sum = 0;
    for (size_t i = 0; i < (size_t)num_samples; ++i) {
        sum += data[i] * data[i];
    }
    return std::sqrt(sum / num_samples);
}

inline float rms(const float *data, int num_samples) {
    float sum = 0;
    for (size_t i = 0; i < (size_t)num_samples; ++i) {
        sum += data[i] * data[i];
    }
    return std::sqrt(sum / (float)num_samples);
}

inline double peak(const double *data, int num_samples) {
    double max_val = 0;
    for (size_t i = 0; i < (size_t)num_samples; ++i) {
        max_val = std::max(max_val, std::abs(data[i]));
    }
    return max_val;
}

inline float peak(const float *data, int num_samples) {
    float max_val = 0;
    for (size_t i = 0; i < (size_t)num_samples; ++i) {
        max_val = std::max(max_val, std::abs(data[i]));
    }
    return max_val;
}

inline double sigmoid(double xn, double knee = 1) {
    return xn / (1 + std::abs(knee * xn));
}

inline double cubicClip(double xn) {
    if (std::abs(xn) > 1)
        return sign(xn);
    return xn - (xn * xn * xn) / 3.;
}

// Windowing functions and generators

// n: sample index [0, N-1)
// N: window length

constexpr double hann(int n, int N) {
    return 0.5 * (1.0 - std::cos(2 * M_PI * n / (N - 1)));
}

template <int window_size>
constexpr std::array<double, window_size> hannWindow = [] {
    std::array<double, window_size> window{};
    for (int n = 0; n < window_size; ++n)
        window[n] = hann(n, window_size);
    return window;
}();

constexpr double hamming(int n, int N) {
    return 0.54 - 0.46 * std::cos(2. * M_PI * n / (N - 1));
}

template <int window_size>
constexpr std::array<double, window_size> hammingWindow = [] {
    std::array<double, window_size> window{};
    for (int n = 0; n < window_size; ++n)
        window[n] = hamming(n, window_size);
    return window;
}();

constexpr double blackman(int n, int N) {
    constexpr double a0  = 0.42;
    constexpr double a1  = 0.5;
    constexpr double a2  = 0.08;
    double           arg = 2.0 * M_PI * n / (N - 1);
    return a0 - a1 * std::cos(arg) + a2 * std::cos(2. * arg);
}

template <int window_size>
constexpr std::array<double, window_size> blackmanWindow = [] {
    std::array<double, window_size> window{};
    for (int n = 0; n < window_size; ++n)
        window[n] = blackman(n, window_size);
    return window;
}();

constexpr double bartlet(int n, int N) {
    return 1 - std::abs(2. * n / (N - 1) - 1.);
}

template <int window_size>
constexpr std::array<double, window_size> bartletWindow = [] {
    std::array<double, window_size> window{};
    for (int n = 0; n < window_size; ++n)
        window[n] = bartlet(n, window_size);
    return window;
}();

} // namespace omni
