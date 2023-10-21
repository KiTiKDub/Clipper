/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
ClipperAudioProcessor::ClipperAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
    bypass = dynamic_cast<juce::AudioParameterBool*>(apvts.getParameter("bypass"));
    clipSelect = dynamic_cast<juce::AudioParameterInt*>(apvts.getParameter("clipSelect"));
    oversampleSelect = dynamic_cast<juce::AudioParameterInt*>(apvts.getParameter("oversampleSelect"));
    threshold = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("threshold"));
    inValue = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("inValue"));
    outValue = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("outValue"));
    
}

ClipperAudioProcessor::~ClipperAudioProcessor()
{
}

//==============================================================================
const juce::String ClipperAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool ClipperAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool ClipperAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool ClipperAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double ClipperAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int ClipperAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int ClipperAudioProcessor::getCurrentProgram()
{
    return 0;
}

void ClipperAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String ClipperAudioProcessor::getProgramName (int index)
{
    return {};
}

void ClipperAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void ClipperAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.maximumBlockSize = samplesPerBlock;
    spec.sampleRate = sampleRate;
    spec.numChannels = getTotalNumInputChannels();

    inGain.reset();
    inGain.prepare(spec);
    inGain.setRampDurationSeconds(.05);

    outGain.reset();
    outGain.prepare(spec);
    outGain.setRampDurationSeconds(.05);

    for (auto& oversample : overSamplers)
    {
        oversample.reset();
        oversample.initProcessing(samplesPerBlock);
    }

}

void ClipperAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool ClipperAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void ClipperAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    if (bypass->get())
        return;

    auto inputBlock = juce::dsp::AudioBlock<float>(buffer);
    auto inputContext = juce::dsp::ProcessContextReplacing<float>(inputBlock);

    inGain.setGainDecibels(inValue->get());
    inGain.process(inputContext);

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    auto ovRate = oversampleSelect->get();

    auto ovBlock = overSamplers[ovRate].processSamplesUp(inputBlock);
    auto ovContext = juce::dsp::ProcessContextReplacing<float>(ovBlock);

    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelInput = ovContext.getInputBlock().getChannelPointer(channel);
        auto* channelOutput = ovContext.getOutputBlock().getChannelPointer(channel);

        auto len = ovContext.getInputBlock().getNumSamples();

        if (clipSelect->get() == 1) //Hard Clipper
        {
            for (int s = 0; s < len; ++s)
            {
                auto gain = juce::Decibels::decibelsToGain(threshold->get());
                channelInput[s] > gain ? channelOutput[s] = gain : channelOutput[s] = channelInput[s];
                channelInput[s] < -gain ? channelOutput[s] = -gain : channelOutput[s] = channelInput[s];
            }
        }

        else if (clipSelect->get() == 2) //Soft Clipper (cubixc)
        {
            for (int s = 0; s < len; ++s)
            {
                auto newLimit = juce::Decibels::decibelsToGain(threshold->get());
                auto inverse = 1 / newLimit;
                auto resizeSamples = channelInput[s] * inverse;
                resizeSamples > 1 ? resizeSamples = 1 : resizeSamples = resizeSamples;
                resizeSamples < -1 ? resizeSamples = -1 : resizeSamples = resizeSamples;
                auto cubic = (resizeSamples - pow(resizeSamples, 3) / 3);

                channelOutput[s] = cubic * newLimit;
            }
        }

        else if (clipSelect->get() == 3) //sin clipping, 
        {
            for (int s = 0; s < len; ++s)
            {
                auto newLimit = juce::Decibels::decibelsToGain(threshold->get());
                auto inverse = 1 / newLimit;
                auto resizeSamples = channelInput[s] * inverse;
                resizeSamples > 1 ? resizeSamples = 1 : resizeSamples = resizeSamples;
                resizeSamples < -1 ? resizeSamples = -1 : resizeSamples = resizeSamples;

                auto sinosidal = sin(3 * juce::MathConstants<float>::pi * resizeSamples / 4);

                channelOutput[s] = sinosidal * newLimit;
            }

        }

        else if (clipSelect->get() == 4) //hyperbolic tangent
        {
            for (int s = 0; s < len; ++s)
            {
                auto newLimit = juce::Decibels::decibelsToGain(threshold->get());
                auto inverse = 1 / newLimit;
                auto resizeSamples = channelInput[s] * inverse;
                resizeSamples > 1 ? resizeSamples = 1 : resizeSamples = resizeSamples;
                resizeSamples < -1 ? resizeSamples = -1 : resizeSamples = resizeSamples;

                auto hyperTan = tanh(5 * resizeSamples) * (3 / juce::MathConstants<float>::pi);

                channelOutput[s] = hyperTan * newLimit;
            }
        }

        else if (clipSelect->get() == 5) //arctangent tangent
        {
            for (int s = 0; s < len; ++s)
            {
                auto newLimit = juce::Decibels::decibelsToGain(threshold->get());
                auto inverse = 1 / newLimit;
                auto resizeSamples = channelInput[s] * inverse;
                resizeSamples > 1 ? resizeSamples = 1 : resizeSamples = resizeSamples;
                resizeSamples < -1 ? resizeSamples = -1 : resizeSamples = resizeSamples;

                auto hyperTan = atan(5 * resizeSamples) * (2 / juce::MathConstants<float>::pi);

                channelOutput[s] = hyperTan * newLimit;
            }
        }

        else if (clipSelect->get() == 6) //Soft Clipper (quntic)
        {
            for (int s = 0; s < len; ++s)
            {
                auto newLimit = juce::Decibels::decibelsToGain(threshold->get());
                auto inverse = 1 / newLimit;
                auto resizeSamples = channelInput[s] * inverse;
                resizeSamples > 1 ? resizeSamples = 1 : resizeSamples = resizeSamples;
                resizeSamples < -1 ? resizeSamples = -1 : resizeSamples = resizeSamples;
                auto quintic = resizeSamples - pow(resizeSamples, 5) / 5;

                channelOutput[s] = quintic * newLimit;
            }
        }
    }
    auto& outputBlock = inputContext.getOutputBlock();
    overSamplers[ovRate].processSamplesDown(outputBlock);

    outGain.setGainDecibels(outValue->get());
    outGain.process(inputContext);
}

//==============================================================================
bool ClipperAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* ClipperAudioProcessor::createEditor()
{
    //return new ClipperAudioProcessorEditor (*this);
    return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void ClipperAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::MemoryOutputStream mos(destData, true);
    apvts.state.writeToStream(mos);
}

void ClipperAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto tree = juce::ValueTree::readFromData(data, sizeInBytes);
    if (tree.isValid()) {
        apvts.replaceState(tree);
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout ClipperAudioProcessor::createParameterLayout()
{
    using namespace juce;

    AudioProcessorValueTreeState::ParameterLayout layout;

    auto threshRange = NormalisableRange<float>(-60, 0, .1, 1);
    auto gainRange = NormalisableRange<float>(-20, 20, .1, 1);

    layout.add(std::make_unique<AudioParameterBool>("bypass", "Bypass", false));
    layout.add(std::make_unique<AudioParameterFloat>("inValue", "In Gain", gainRange, 0));
    layout.add(std::make_unique<AudioParameterInt>("clipSelect", "Clipper Select", 1, 6, 1));
    layout.add(std::make_unique<AudioParameterInt>("oversampleSelect", "OverSample Factor", 0, 3, 2));
    layout.add(std::make_unique<AudioParameterFloat>("threshold", "Threshold", threshRange, 0));
    layout.add(std::make_unique<AudioParameterFloat>("outValue", "Out Gain", gainRange, 0));

    return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ClipperAudioProcessor();
}
