#pragma once

#include "filter.h"
#include <algorithm>

namespace omni {

class Distortion {
  public:
    virtual double distortion(double xn) = 0;

    double processSample(double xn) {
        double drive_bias = xn * driveMag + bias;
        double yn         = distortion(drive_bias);
        if (dcBlock) {
            yn = dc_block.processSample(yn);
        }
        return yn;
    }

    void setDrive(double _driveMag) { driveMag = _driveMag; }
    void setBias(double _bias) { bias = _bias; }
    void useDcBlock(bool _dcBlock) { dcBlock = _dcBlock; }

  protected:
    double driveMag = 1;
    double bias     = 0;
    bool   dcBlock  = false;

    DCBlocker dc_block;
};

/* ======================================================== */

class RectifierFull : public Distortion {
  public:
    double distortion(double xn) override {
        if (xn < 0.f)
            return -xn;
        return xn;
    }
};

class RectifierHalf : public Distortion {
  public:
    double distortion(double xn) override {
        if (xn < 0.f)
            return 0.f;
        return xn;
    }
};

class SineFold : public Distortion {
  public:
    double distortion(double xn) override { return std::sin(M_2_PI * xn); }
};

class HardClip : public Distortion {
  public:
    double distortion(double xn) override { return std::clamp(xn, -1., 1.); }
};

class TanhShaper : public Distortion {
  public:
    double distortion(double xn) override { return std::tanh(xn); }
};

class SoftClipper : public Distortion {
  public:
    void setThreshold_linear(double threshold) { threshold_linear = threshold; }

    void setThreshold_dB(double threshold) {
        threshold_linear = std::pow(10.0, threshold / 20.0);
    }

    void setKnee_linear(double knee) {
        knee_linear = std::min(2 * threshold_linear, knee);
        halfKnee    = 0.5 * knee_linear;
    }

    void setKnee_dB(double knee) {
        knee_linear = std::min((2 * threshold_linear),
                               (1.0 / std::pow(10.0, knee / 20.0)));
        halfKnee    = 0.5 * knee_linear;
    }

    double distortion(double xn) override {
        double abs  = std::fabs(xn);
        int    sign = xn >= 0 ? 1 : -1;

        if (abs <= threshold_linear - halfKnee) {
            return xn;
        } else if (abs <= threshold_linear - halfKnee) {
            double excess = abs - (threshold_linear - halfKnee);
            return sign * (abs - (excess * excess) / (2 * knee_linear));
        } else {
            return sign * threshold_linear;
        }
    }

  private:
    double threshold_linear;
    double knee_linear;
    double halfKnee;
};

} // namespace omni
