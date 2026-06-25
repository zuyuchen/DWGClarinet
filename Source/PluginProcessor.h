/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "DWGClarinet.h"

//==============================================================================
/**
    MIDI-driven monophonic clarinet synth, Stage 1–2 (linear ReedTable,
    scalar bell reflection). One DWGClarinet voice; new note-on retunes and
    re-triggers the breath envelope on the same instance (monophonic, like
    a real single-reed instrument — you can't play two notes on one reed).
*/
class DWGClarinetAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    DWGClarinetAudioProcessor();
    ~DWGClarinetAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    // Exposed so the editor can wire up knobs without needing an APVTS yet
    // (Stage 1–2 has few enough parameters that a full AudioProcessorValueTreeState
    // is unnecessary overhead; revisit once Bernoulli/dynamic reed adds more params).
    juce::AudioParameterFloat* reedOffsetParam  = nullptr;
    juce::AudioParameterFloat* reedSlopeParam   = nullptr;
    juce::AudioParameterFloat* bellReflectParam = nullptr;

private:
    //==============================================================================
    DWGClarinet clarinet;

    // ── Monophonic note tracking ─────────────────────────────────────────────
    // Real-world clarinet: one reed, one note at a time. If a second key is
    // pressed while the first is held, the new note re-triggers (retunes the
    // existing voice) rather than spawning a second voice. Releasing the
    // second note falls back to the first if it's still held (basic note
    // stack), matching how monophonic synths conventionally behave.
    std::vector<int> heldNotes;
    int currentMidiNote = -1;

    static float midiNoteToFreq (int midiNote)
    {
        return 440.0f * std::pow (2.0f, (midiNote - 69) / 12.0f);
    }

    void handleNoteOn  (int midiNote, float velocity);
    void handleNoteOff (int midiNote);

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DWGClarinetAudioProcessor)
};
