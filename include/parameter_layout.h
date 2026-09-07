/*
  ==============================================================================

    ParameterLayout.h
    Created: 3 Mar 2026 3:16:38pm
    Author:  Nico Russo

  ==============================================================================
*/

#pragma once

#define SCFloat static constexpr float
#define SCInt static constexpr int
#define SCBool static constexpr bool
#define SCString static constexpr const char *

struct Skew {
    SCFloat linear = 1.0f;
    SCFloat exponential = 3.f;
    SCFloat logarithmic = 0.3f;
};

#include <juce_audio_processors/juce_audio_processors.h>

void addFloat(juce::AudioProcessorValueTreeState::ParameterLayout &layout,
              const char *ID, const char *name, const float min,
              const float max, const float defaultValue, const float stepSize,
              const float skew = 1.f, const char *suffix = "") {
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ID, 1), name,
        juce::NormalisableRange<float>(min, max, stepSize, skew), defaultValue,
        suffix));
}

void addBool(juce::AudioProcessorValueTreeState::ParameterLayout &layout,
             const char *ID, const char *name, const bool defaultValue) {
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID(ID, 1), name, defaultValue));
}

void addInt(juce::AudioProcessorValueTreeState::ParameterLayout &layout,
            const char *ID, const char *name, const int min, const int max,
            const int defaultValue, const char *suffix = "") {
    layout.add(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID(ID, 1), name, min, max, defaultValue, suffix));
}

void addChoice(juce::AudioProcessorValueTreeState::ParameterLayout &layout,
               const char *ID, const char *name, juce::StringArray choices,
               const int defaultValue) {
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID(ID, 1), name, choices, defaultValue));
}

class pdcFloat {
  public:
    pdcFloat() { apvts = nullptr; }

    void prepare(double _sampleRate, int _blockSize,
                 juce::AudioProcessorValueTreeState *_apvts, const char *_ID,
                 double _smoothRate = 0.02) {
        smoothRate = _smoothRate;
        ID = _ID;
        apvts = _apvts;
        reset(_sampleRate, _blockSize);
    }

    void reset(double _sampleRate, int _blockSize) {
        fs = _sampleRate;
        blockSize = _blockSize;

        smoothValue.reset(fs, smoothRate);
        smoothValue.setCurrentAndTargetValue(
            apvts->getRawParameterValue(ID)->load());
    }

    float getRaw() { return rawValue; }

    float getPrev() { return prevValue; }

    bool changed() { return std::abs(rawValue - prevValue) > 0.001f; }

    float getRawAndUpdate() {
        prevValue = rawValue;
        rawValue = apvts->getRawParameterValue(ID)->load();
        smoothValue.setTargetValue(rawValue);
        return rawValue;
    }

    void update() {
        prevValue = rawValue;
        float newRaw = apvts->getRawParameterValue(ID)->load();
        if (newRaw != rawValue) {
            rawValue = newRaw;
            smoothValue.setTargetValue(rawValue);
        }
    }

    float getNextValue(int skip = 0) {
        if (skip == 0) {
            return smoothValue.getNextValue();
        } else {
            return smoothValue.skip(skip);
        }
    }

    float getCurrentValue() { return smoothValue.getCurrentValue(); }

  private:
    juce::AudioProcessorValueTreeState *apvts;
    juce::SmoothedValue<float> smoothValue;
    const char *ID;
    float rawValue;
    float prevValue;
    double fs;
    double smoothRate;
    int blockSize;
};

class pdcBool {
  public:
    void prepare(double _sampleRate, int _blockSize,
                 juce::AudioProcessorValueTreeState *_apvts, const char *_ID) {
        ID = _ID;
        apvts = _apvts;
        reset(_sampleRate, _blockSize);
    }

    void reset(double _sampleRate, int _blockSize) {
        fs = _sampleRate;
        blockSize = _blockSize;

        param = apvts->getRawParameterValue(ID);
    }

    bool getNextValue() {
        prevValue = rawValue;
        rawValue = param->load() > 0.5f;
        return rawValue;
    }

    bool getCurrentValue() { return rawValue; }

    bool changed() { return rawValue != prevValue; }

  private:
    juce::AudioProcessorValueTreeState *apvts;
    std::atomic<float> *param = nullptr;
    const char *ID;
    bool rawValue;
    bool prevValue;
    double fs;
    int blockSize;
};

class pdcInt {
  public:
    void prepare(double _sampleRate, int _blockSize,
                 juce::AudioProcessorValueTreeState *_apvts, const char *_ID) {
        ID = _ID;
        apvts = _apvts;
        reset(_sampleRate, _blockSize);
    }

    void reset(double _sampleRate, int _blockSize) {
        fs = _sampleRate;
        blockSize = _blockSize;

        param = apvts->getRawParameterValue(ID);
    }

    int getNextValue() {
        prevValue = rawValue;
        rawValue = static_cast<int>(param->load());
        return rawValue;
    }

    int getCurrentValue() { return rawValue; }

    bool changed() { return rawValue != prevValue; }

  private:
    juce::AudioProcessorValueTreeState *apvts;
    std::atomic<float> *param = nullptr;
    const char *ID;
    int rawValue;
    int prevValue;
    double fs;
    int blockSize;
};
