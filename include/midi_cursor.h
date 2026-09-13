#pragma once

#include "juce_audio_basics/juce_audio_basics.h"

namespace omni {

class MidiCursor {
  public:
    MidiCursor(const juce::MidiBuffer &midi)
        : current(midi.begin()), end(midi.end()) {}

    bool hasEvent() const { return current != end; }

    const juce::MidiMessageMetadata event() const { return *current; }

    void advance() { ++current; }

  private:
    juce::MidiBufferIterator current;
    juce::MidiBufferIterator end;
};

} // namespace omni
