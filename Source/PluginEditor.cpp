/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
DWGClarinetAudioProcessorEditor::DWGClarinetAudioProcessorEditor (DWGClarinetAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    auto setupSlider = [this] (juce::Slider& slider, juce::Label& label,
                                const juce::String& name,
                                float minVal, float maxVal, float startVal)
    {
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setRange (minVal, maxVal, 0.001);
        slider.setValue (startVal, juce::dontSendNotification);
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 20);
        slider.addListener (this);
        addAndMakeVisible (slider);

        label.setText (name, juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centred);
        label.attachToComponent (&slider, false);
        addAndMakeVisible (label);
    };

    setupSlider (reedOffsetSlider,  reedOffsetLabel,  "Reed Offset",
                 audioProcessor.reedOffsetParam->range.start,
                 audioProcessor.reedOffsetParam->range.end,
                 audioProcessor.reedOffsetParam->get());

    setupSlider (reedSlopeSlider,   reedSlopeLabel,   "Reed Slope",
                 audioProcessor.reedSlopeParam->range.start,
                 audioProcessor.reedSlopeParam->range.end,
                 audioProcessor.reedSlopeParam->get());

    setupSlider (bellReflectSlider, bellReflectLabel, "Bell Reflection",
                 audioProcessor.bellReflectParam->range.start,
                 audioProcessor.bellReflectParam->range.end,
                 audioProcessor.bellReflectParam->get());

    setSize (420, 160);
}

DWGClarinetAudioProcessorEditor::~DWGClarinetAudioProcessorEditor()
{
}

//==============================================================================
void DWGClarinetAudioProcessorEditor::sliderValueChanged (juce::Slider* slider)
{
    if (slider == &reedOffsetSlider)
        *audioProcessor.reedOffsetParam = (float) reedOffsetSlider.getValue();
    else if (slider == &reedSlopeSlider)
        *audioProcessor.reedSlopeParam = (float) reedSlopeSlider.getValue();
    else if (slider == &bellReflectSlider)
        *audioProcessor.bellReflectParam = (float) bellReflectSlider.getValue();
}

//==============================================================================
void DWGClarinetAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (15.0f));
    g.drawFittedText ("DWGClarinet — Stage 1/2", getLocalBounds().removeFromTop (30),
                       juce::Justification::centred, 1);
}

void DWGClarinetAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced (10);
    bounds.removeFromTop (30); // space for title text drawn in paint()

    const int knobWidth = bounds.getWidth() / 3;
    reedOffsetSlider.setBounds  (bounds.removeFromLeft (knobWidth).reduced (10, 20));
    reedSlopeSlider.setBounds   (bounds.removeFromLeft (knobWidth).reduced (10, 20));
    bellReflectSlider.setBounds (bounds.reduced (10, 20));
}
