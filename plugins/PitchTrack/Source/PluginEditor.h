/*
  ==============================================================================

    This file was auto-generated by the Introjucer!

    It contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "PluginProcessor.h"

using namespace gin;
//==============================================================================
/**
*/
class PitchTrackAudioProcessorEditor : public GinAudioProcessorEditor,
                                       private Timer
{
public:
    PitchTrackAudioProcessorEditor (PitchTrackAudioProcessor&);
    ~PitchTrackAudioProcessorEditor() override;

    //==============================================================================
    void resized() override;
    void paint(Graphics& g) override;
    void timerCallback() override;

private:
    PitchTrackAudioProcessor& proc;
    
    float lastPitch = -1;
    Label pitch;
    LevelMeter meter;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PitchTrackAudioProcessorEditor)
};
