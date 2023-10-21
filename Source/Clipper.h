/*
  ==============================================================================

    Clipper.h
    Created: 21 Oct 2023 1:37:58pm
    Author:  kylew

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

struct Clipper 
{
    void prepareToPlay(double sampleRate, int samplesPerBlock, int numChannels);
    void process(juce::AudioBuffer<float>& buffer);
};