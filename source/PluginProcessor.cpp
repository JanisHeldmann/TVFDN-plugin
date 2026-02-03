/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"


//==============================================================================
GlivelabPlugin64AudioProcessor::GlivelabPlugin64AudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput ("Input", juce::AudioChannelSet::discreteChannels(64), true)
                      #endif
                     .withOutput ("Output", juce::AudioChannelSet::discreteChannels(64), true)
                     #endif
                       )
#endif
{
}


GlivelabPlugin64AudioProcessor::~GlivelabPlugin64AudioProcessor()
{
}

//==============================================================================



const juce::String GlivelabPlugin64AudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool GlivelabPlugin64AudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool GlivelabPlugin64AudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool GlivelabPlugin64AudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double GlivelabPlugin64AudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int GlivelabPlugin64AudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int GlivelabPlugin64AudioProcessor::getCurrentProgram()
{
    return 0;
}

void GlivelabPlugin64AudioProcessor::setCurrentProgram (int index)
{
}

const juce::String GlivelabPlugin64AudioProcessor::getProgramName (int index)
{
    return {};
}

void GlivelabPlugin64AudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void GlivelabPlugin64AudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{

    juce::dsp::ProcessSpec  spec;
    juce::dsp::ProcessSpec  filterSpec;
    
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = getTotalNumInputChannels();
    filterSpec = spec;
    filterSpec.numChannels = 1;
    
    count++;
    if (count == 1)
        fdn.prepare(spec,filterSpec);
    
}
void GlivelabPlugin64AudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool GlivelabPlugin64AudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return true;

}
#endif

void GlivelabPlugin64AudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{

    juce::ScopedNoDenormals noDenormals;

    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    juce::dsp::AudioBlock<float> block (buffer);

    fdn.RT_DC  = *apvts.getRawParameterValue("RT_DC");
    fdn.RT_NY =  *apvts.getRawParameterValue("RT_NY");
    fdn.RT_CrossOverFrequency =  *apvts.getRawParameterValue("RT_CrossOverFrequency");
    fdn.TVBypassed = *apvts.getRawParameterValue("TV Bypassed");
    fdn.AbsorptionBypassed = *apvts.getRawParameterValue("Absorption Bypassed");
    fdn.osc_frequency  = *apvts.getRawParameterValue("Osc_Frequency");
    fdn.delayFactor = *apvts.getRawParameterValue("Delay_Factor");
    fdn.spread = *apvts.getRawParameterValue("Frequency Spread");

   fdn.process(block);
   
}


//==============================================================================
bool GlivelabPlugin64AudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* GlivelabPlugin64AudioProcessor::createEditor()
{
//   return new GlivelabPlugin64AudioProcessorEditor (*this);
   return new GlivelabPlugin64AudioProcessorEditor(*this);
}

//==============================================================================
void GlivelabPlugin64AudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void GlivelabPlugin64AudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

juce::AudioProcessorValueTreeState::ParameterLayout GlivelabPlugin64AudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    
    layout.add(std::make_unique<juce::AudioParameterFloat>("RT_DC",
                                                           "RT_DC",
                                                           juce::NormalisableRange<float>(0.5f,10.f,0.1f,1.f),
                                                           3.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("RT_NY",
                                                           "RT_NY",
                                                           juce::NormalisableRange<float>(0.5f,10.f,0.1f,1.f),
                                                           1.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("RT_CrossOverFrequency",
                                                           "RT_CrossOverFrequency",
                                                           juce::NormalisableRange<float>(100.f,8000.f,100.f,1.f),
                                                           1.000f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("Osc_Frequency",
                                                           "Osc_Frequency",
                                                           juce::NormalisableRange<float>(0.1f,10.f,0.1f,1.f),
                                                           1.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("Delay_Factor",
                                                           "Delay_Factor",
                                                           juce::NormalisableRange<float>(0.5f,5.f,0.1f,1.f),
                                                           1.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("Frequency Spread",
                                                           "Frequency Spread",
                                                           juce::NormalisableRange<float>(0.1f,1.f,0.1f,1.f),
                                                           .5f));
    
    layout.add(std::make_unique<juce::AudioParameterBool>("TV Bypassed", "TV Bypassed", false));
    layout.add(std::make_unique<juce::AudioParameterBool>("Absorption Bypassed", "Absorption Bypassed", false));
 
    return layout;
}
//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GlivelabPlugin64AudioProcessor();
}

