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
    void process(juce::dsp::AudioBlock<float>& block, int channel);
    void updateParams(int mode, float threshold);

private:

    enum 
    {
        hardClipper,
        cubic,
        sin,
        hTangent,
        arcTangent,
        quintic
    };

    int clipperMode;
    float clipperThresh;
};