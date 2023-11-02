/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "LevelMeter/LevelMeter.h"

//==============================================================================
/**
*/
class ClipperAudioProcessorEditor  : public juce::AudioProcessorEditor, public juce::Timer
{
public:
    ClipperAudioProcessorEditor (ClipperAudioProcessor&);
    ~ClipperAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    void setRotarySlider(juce::Slider&);

    void timerCallback() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    ClipperAudioProcessor& audioProcessor;

    std::array<LevelMeter, 2> meter;
    std::array<LevelMeter, 2> outMeter;

    juce::Slider inGain       { "In Gain" },
                 oversampling { "Oversampling" },
                 outGain      { "Out Gain" },
                 clipCeiling  { "Clip Ceiling" },
                 clipType     { "Clip Type" };

    juce::ToggleButton bypass { "Bypass" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ClipperAudioProcessorEditor)
};
