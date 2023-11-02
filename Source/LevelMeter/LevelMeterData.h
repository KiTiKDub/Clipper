#pragma once
#include <JuceHeader.h>

struct LevelMeterData
{
	void process(bool inOrOut, int channel, juce::AudioBuffer<float>& buffer);
	float getRMS(int channel);
	float getOutRMS(int channel);

private:

	std::array<std::atomic<float>, 2> rmsIn;
	std::array<std::atomic<float>, 2> rmsOut;
};

void LevelMeterData::process(bool inOrOut, int channel, juce::AudioBuffer<float>& buffer)
{
	if (inOrOut) //true is in rms, false is out rms
	{
		rmsIn[channel] = juce::Decibels::gainToDecibels(buffer.getRMSLevel(channel, 0, buffer.getNumSamples()));
	}
	else
	{
		rmsOut[channel] = juce::Decibels::gainToDecibels(buffer.getRMSLevel(channel, 0, buffer.getNumSamples()));
	}
}

float LevelMeterData::getRMS(int channel)
{
	return rmsIn[channel];
}

float LevelMeterData::getOutRMS(int channel)
{
	return rmsOut[channel];
}