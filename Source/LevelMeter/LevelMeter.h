/*
  ==============================================================================

    LevelMeter.h
    Created: 22 Oct 2023 4:02:22pm
    Author:  kylew

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

struct LevelMeter : juce::Component
{
    void paint(juce::Graphics& g) override;

    //default value so the meters  are black when the plugin is launched
    void setLevel(float value) { level = value; }

private:
    float level = -60.f;
};

void LevelMeter::paint(juce::Graphics& g)
{
    using namespace juce;

    //shapes the meters. May be a bit inefficeint, not sure the best way to move this stuff around, but it is there.
    auto bounds = getLocalBounds().toFloat();
    bounds = bounds.removeFromLeft(bounds.getWidth() * .8);
    bounds = bounds.removeFromRight(bounds.getWidth() * .75);
    bounds = bounds.removeFromTop(bounds.getHeight() * .9);
    bounds = bounds.removeFromBottom(bounds.getHeight() * .88);

    //get our base rectangle
    g.setColour(Colours::grey);
    g.fillRect(bounds);

    //get gradient
    auto gradient = ColourGradient::ColourGradient(Colours::green, bounds.getBottomLeft(), Colours::red, bounds.getTopLeft(), false);
    gradient.addColour(.5f, Colours::yellow);
    g.setGradientFill(gradient);

    //Show gradient
    auto levelMeterFill = jmap(level, -60.f, +6.f, 0.f, static_cast<float>(bounds.getHeight()));
    g.fillRoundedRectangle(bounds.removeFromBottom(levelMeterFill), 5.f);
}

/*
=============================================================================================================
                        PLACE BELOW CODE IN YOUR timerCallback() FUNCTION
=============================================================================================================

    for (auto channel = 0; channel < audioProcessor.getTotalNumInputChannels(); channel++) {
        meter[channel].setLevel(audioProcessor.levelMeter.getRMS(channel));
        meter[channel].repaint();

        outMeter[channel].setLevel(audioProcessor.levelMeter.getOutRMS(channel));
        outMeter[channel].repaint();
    }
*/