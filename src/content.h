#pragma once

#include "juce/juce.h"
#include "juce/parameter_layout.h"
#include "juce/sample_tap.h"

#include "processor.h"

#include "theme.h"
#include "ui/dropdown.h"
#include "ui/knob.h"
#include "ui/spectrum_view.h"
#include "ui/toggle.h"

class Content : public juce::Component {
  public:
    static constexpr int design_width  = 600;
    static constexpr int design_height = 300;

    // CONSTRUCTOR: initialize widgets
    Content(APVTS &apvts_, const Theme &t, SampleTap &input, SampleTap &output)
        : theme(t), apvts(apvts_),
          // initialize all components
          bypass(theme, getBool(Processor::Params::bypass_ID)),
          input_spectrum(theme, input, false),
          output_spectrum(theme, output, true) {
        //
        // setting initial plugin size
        setSize(design_width, design_height);

        // ADD AND MAKE VISIBLE
        addAndMakeVisible(bypass);
        addAndMakeVisible(input_spectrum);
        addAndMakeVisible(output_spectrum);
    }

    // Draw non-widget elements (background)
    void paint(juce::Graphics &g) override {
        // Background
        g.fillAll(theme.background);
        auto area = getLocalBounds();

        // Plugin Header
        auto header = area.removeFromTop(100);
        g.setColour(theme.panel);
        g.fillRect(header);

        auto title_area = header.reduced(25);
        g.setColour(theme.text);
        g.setFont(Fonts::jetBrainsMonoBold(50));
        g.drawText(
            "PLUGIN TITLE", title_area, juce::Justification::centredRight);
    }

    // Set bounds of all widgets
    void resized() override {
        auto area = getLocalBounds();

        // Header
        auto header       = area.removeFromTop(100);
        auto header_inner = header.reduced(25);
        bypass.setBounds(
            header_inner.removeFromLeft(50).withSizeKeepingCentre(50, 50));

        // Input/Output Spectrum
        auto spectrum_bounds = area.removeFromBottom(200);
        input_spectrum.setBounds(spectrum_bounds);
        output_spectrum.setBounds(spectrum_bounds);
    }

    // Update all widgets that require sample rate
    void frame(double fs) {
        input_spectrum.update(fs);
        output_spectrum.update(fs);
    }

  private:
    juce::AudioParameterBool &getBool(const char *id) {
        auto *p =
            dynamic_cast<juce::AudioParameterBool *>(apvts.getParameter(id));
        jassert(p != nullptr);
        return *p;
    }

    static juce::AudioParameterChoice &getChoice(APVTS &apvts, const char *id) {
        auto *p =
            dynamic_cast<juce::AudioParameterChoice *>(apvts.getParameter(id));
        jassert(p != nullptr);
        return *p;
    }

    static juce::AudioParameterInt &getInt(APVTS &apvts, const char *id) {
        auto *p =
            dynamic_cast<juce::AudioParameterInt *>(apvts.getParameter(id));
        jassert(p != nullptr);
        return *p;
    }

    static juce::RangedAudioParameter &getFloat(APVTS &apvts, const char *id) {
        auto *p = apvts.getParameter(id);
        jassert(p != nullptr);
        return *p;
    }

  private:
    const Theme &theme;
    APVTS       &apvts;

    // UI Components
    BypassToggle bypass;

    SpectrumView<4096> input_spectrum;
    SpectrumView<4096> output_spectrum;
};
