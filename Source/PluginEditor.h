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
    Minimal Stage 1–2 test UI: three knobs (reed offset, reed slope, bell
    reflection) wired directly to the processor's plain AudioParameterFloat
    members. No APVTS attachment objects yet — direct getValue/setValueNotifyingHost
    wiring is enough for this small a parameter set; revisit if/when Stage 3+
    adds enough parameters to justify the APVTS + GenericAudioProcessorEditor
    machinery.
*/
class DWGClarinetAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                         private juce::Slider::Listener
{
public:
    DWGClarinetAudioProcessorEditor (DWGClarinetAudioProcessor&);
    ~DWGClarinetAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void sliderValueChanged (juce::Slider* slider) override;

    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    DWGClarinetAudioProcessor& audioProcessor;

    juce::Slider reedOffsetSlider, reedSlopeSlider, bellReflectSlider;
    juce::Label  reedOffsetLabel,  reedSlopeLabel,  bellReflectLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DWGClarinetAudioProcessorEditor)
};
