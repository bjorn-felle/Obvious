/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "CommandDefinitions.h"

//==============================================================================
/**
*/

typedef enum : int {
    EditorLayoutPurposeResized,
    EditorLayoutPurposeUIActivity,
} EditorLayoutPurpose;


class ObviousAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:    
    ObviousAudioProcessorEditor (ObviousAudioProcessor&, juce::AudioProcessorValueTreeState&);
    ~ObviousAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    void layout(EditorLayoutPurpose purpose);

private:
    
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    ObviousAudioProcessor& audioProcessor;
    
    int numItems();
//    
    bool hitTest (int x, int y) override;
        
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ObviousAudioProcessorEditor)
};
