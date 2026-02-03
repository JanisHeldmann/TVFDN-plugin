/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class GlivelabPlugin64AudioProcessorEditor  : public juce::GenericAudioProcessorEditor
{
public:
    GlivelabPlugin64AudioProcessorEditor (GlivelabPlugin64AudioProcessor&);
    ~GlivelabPlugin64AudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    GlivelabPlugin64AudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GlivelabPlugin64AudioProcessorEditor)
};
