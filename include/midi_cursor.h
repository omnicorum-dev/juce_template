/*
  ==============================================================================

    midi_cursor.h
    Created: 8 Sep 2026 3:58:38pm
    Author:  Nico Russo

  ==============================================================================
*/

#pragma once

#include "juce_audio_basics/juce_audio_basics.h"

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
