#include "processor.h"
#include "editor.h"
#include "juce/midi_cursor.h"
#include "juce_audio_basics/juce_audio_basics.h"
#include "juce_audio_processors_headless/juce_audio_processors_headless.h"
#include "juce_core/juce_core.h"

/* ======================================================== */

Processor::Processor()
    : juce::AudioProcessor(
          BusesProperties()
#if !JucePlugin_IsMidiEffect
#if !JucePlugin_IsSynth
              .withInput("Input", juce::AudioChannelSet::stereo(), true)
              .withInput("Sidechain", juce::AudioChannelSet::stereo(), true)
#endif
              .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
              ),
      apvts(*this, nullptr, "Parameters", createParameterLayout()) {
}

Processor::~Processor() = default;

/* ======================================================== */

APVTS::ParameterLayout Processor::createParameterLayout() {
    APVTS::ParameterLayout layout;

    addFloat(layout,
             Params::inGain_ID,
             Params::inGain_name,
             Params::inGain_min,
             Params::inGain_max,
             Params::inGain_default,
             Params::inGain_stepSize,
             Params::inGain_skew,
             Params::inGain_suffix);

    addFloat(layout,
             Params::outGain_ID,
             Params::outGain_name,
             Params::outGain_min,
             Params::outGain_max,
             Params::outGain_default,
             Params::outGain_stepSize,
             Params::outGain_skew,
             Params::outGain_suffix);

    addFloat(layout,
             Params::mix_ID,
             Params::mix_name,
             Params::mix_min,
             Params::mix_max,
             Params::mix_default,
             Params::mix_stepSize,
             Params::mix_skew,
             Params::mix_suffix);

    addBool(
        layout, Params::bypass_ID, Params::bypass_name, Params::bypass_default);

    return layout;
}

/* ======================================================== */

void Processor::prepareToPlay(double sample_rate, int buffer_size) {
    // if there are dsp objects with latency, gather them here
    const int latency = 0;

    setLatencySamples(latency);
    dry_wet.prepare(sample_rate, latency, bypassParam.getNextValue());

    // prepare parameter smoothers
    outGainSmooth.prepare(sample_rate, buffer_size, &apvts, Params::outGain_ID);
    inGainSmooth.prepare(sample_rate, buffer_size, &apvts, Params::inGain_ID);
    mixSmooth.prepare(sample_rate, buffer_size, &apvts, Params::mix_ID);

    bypassParam.prepare(sample_rate, buffer_size, &apvts, Params::bypass_ID);
}

void Processor::releaseResources() {}

// If you want a custom editor, swap the commented line
juce::AudioProcessorEditor *Processor::createEditor() {
    return new juce::GenericAudioProcessorEditor(*this);
    // return new Editor(*this, apvts);
}

void Processor::processBlock(juce::AudioBuffer<float> &buffer,
                             juce::MidiBuffer         &messages) {

    juce::ScopedNoDenormals no_denormals;

    size_t total_input_channels  = (size_t)getMainBusNumInputChannels();
    size_t total_output_channels = (size_t)getMainBusNumOutputChannels();
    size_t num_samples           = (size_t)buffer.getNumSamples();

    for (auto i = total_input_channels; i < total_output_channels; ++i) {
        buffer.clear((int)i, 0, buffer.getNumSamples());
    }

    /* ======================================================== */

    // Read control-rate parameters
    bool bypass = bypassParam.getNextValue();

    /* ======================================================== */

    // Update smoothers
    outGainSmooth.update();
    inGainSmooth.update();
    mixSmooth.update();

    /* ======================================================== */

    dry_wet.setBypassed(bypass);

    /* ======================================================== */

    // Update objects for discrete changes

    /* ======================================================== */

    MidiCursor midi(messages);

    constexpr size_t max_channels = 8;

    // main input/output buffer
    auto main_io = getBusBuffer(buffer, false, 0);
    // sidechain input buffer
    // (0 channels if host-disabled)
    auto sidechain = getBusBuffer(buffer, true, 1);

    size_t num_channels    = (size_t)main_io.getNumChannels();
    size_t num_sc_channels = (size_t)sidechain.getNumChannels();

    std::array<float *, max_channels>       channel_ptrs;
    std::array<const float *, max_channels> sc_ptrs;

    for (size_t channel = 0; channel < num_channels; ++channel) {
        channel_ptrs[channel] = main_io.getWritePointer((int)channel);
    }
    for (size_t channel = 0; channel < num_sc_channels; ++channel) {
        sc_ptrs[channel] = sidechain.getReadPointer((int)channel);
    }

    /* ======================================================== */

    // SAMPLE/CHANNEL LOOP

    for (size_t sample = 0; sample < num_samples; ++sample) {

        while (midi.hasEvent() && midi.event().samplePosition == (int)sample) {
            juce::MidiMessage message = midi.event().getMessage();

            /* ======================================================== */

            // apply changes based on the midi message received this sample

            /* ======================================================== */

            midi.advance();
        }

        float in_gain  = std::pow(10.f, inGainSmooth.getNextValue() / 20.f);
        float out_gain = std::pow(10.f, outGainSmooth.getNextValue() / 20.f);
        dry_wet.tick(mixSmooth.getNextValue() * 0.01);

        /* ======================================================== */

        // Read sample-rate parameters

        /* ======================================================== */

        // Update objects for continuous changes

        /* ======================================================== */

        for (size_t channel = 0; channel < num_channels; ++channel) {
            float *channel_data = channel_ptrs[channel];
            float  dry          = channel_data[sample];
            float  xn           = dry * in_gain;

            float sc = 0.f;
            if (num_sc_channels > 0) {
                sc = sc_ptrs[std::min(channel, num_sc_channels - 1)][sample];
            }

            if (channel == 0) {
                input_tap.push(xn);
                sidechain_tap.push(sc);
            }

            /* ======================================================== */

            float yn = xn;

            /* ======================================================== */

            double dry_d         = dry_wet.dry(channel, dry);
            double out           = dry_wet.mix(yn, dry_d) * out_gain;
            channel_data[sample] = (float)dry_wet.bypass(out, dry_d);
        }
    }

    output_tap.push(main_io.getReadPointer(0), main_io.getNumSamples());
}

// This might end up clicking, but it does compensate for plugin latency.
// Prefer automating the bypass parameter to host bypass.
void Processor::processBlockBypassed(juce::AudioBuffer<float> &b,
                                     juce::MidiBuffer         &m) {
    dry_wet.setBypassed(true);
    (void)m;

    constexpr size_t max_channels = 8;

    // main input/output buffer
    auto main_io = getBusBuffer(b, false, 0);

    size_t num_channels = (size_t)main_io.getNumChannels();
    size_t num_samples  = (size_t)b.getNumSamples();

    std::array<float *, max_channels> channel_ptrs;

    for (size_t channel = 0; channel < num_channels; ++channel) {
        channel_ptrs[channel] = main_io.getWritePointer((int)channel);
    }

    for (size_t sample = 0; sample < num_samples; ++sample) {
        for (size_t channel = 0; channel < num_channels; ++channel) {
            float *channel_data  = channel_ptrs[channel];
            float  dry           = channel_data[sample];
            channel_data[sample] = (float)dry_wet.dry(channel, dry);
        }
    }
}

/* ======================================================== */

void Processor::getStateInformation(juce::MemoryBlock &dest) {
    auto                              state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, dest);
}

void Processor::setStateInformation(const void *data, int size_in_bytes) {
    std::unique_ptr<juce::XmlElement> xml_state(
        getXmlFromBinary(data, size_in_bytes));
    if (xml_state.get() != nullptr)
        if (xml_state->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xml_state));
}

/* ======================================================== */

juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
    return new Processor();
}
