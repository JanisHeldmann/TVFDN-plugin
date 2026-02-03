/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
GlivelabPlugin64AudioProcessorEditor::GlivelabPlugin64AudioProcessorEditor (GlivelabPlugin64AudioProcessor& p)
    : juce::GenericAudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (400, 300);
}

GlivelabPlugin64AudioProcessorEditor::~GlivelabPlugin64AudioProcessorEditor()
{
}

//==============================================================================
void GlivelabPlugin64AudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (15.0f);
    g.drawFittedText (JucePlugin_VersionString, getLocalBounds(), juce::Justification::bottomRight, 1);
}

void GlivelabPlugin64AudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
}
