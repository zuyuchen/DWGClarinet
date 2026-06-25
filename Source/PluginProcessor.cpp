/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
DWGClarinetAudioProcessor::DWGClarinetAudioProcessor()
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
    // Stage 1–2 parameters — plain AudioParameterFloat for now (no APVTS).
    // Ranges chosen from the validated Stage 1–2 testing:
    //   offset ~0.6–0.9, slope ~ -0.1 to -1.0 keep the reed in/near its
    //   working region (see ReedTable.h header comment on the [-1,+1] clamp
    //   and why saturation at +1 is what drives oscillation).
    {
        addParameter (reedOffsetParam  = new juce::AudioParameterFloat (juce::ParameterID ("reedOffset", 1),  "Reed Offset",  0.3f, 0.95f, 0.7f));
        addParameter (reedSlopeParam   = new juce::AudioParameterFloat (juce::ParameterID ("reedSlope", 1),   "Reed Slope",  -1.5f, -0.05f, -0.3f));
        addParameter (bellReflectParam = new juce::AudioParameterFloat (juce::ParameterID ("bellReflect", 1), "Bell Reflection", -0.999f, -0.5f, -0.95f));
    }
}

DWGClarinetAudioProcessor::~DWGClarinetAudioProcessor()
{
}

//==============================================================================
const juce::String DWGClarinetAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool DWGClarinetAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool DWGClarinetAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool DWGClarinetAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double DWGClarinetAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int DWGClarinetAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int DWGClarinetAudioProcessor::getCurrentProgram()
{
    return 0;
}

void DWGClarinetAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String DWGClarinetAudioProcessor::getProgramName (int index)
{
    return {};
}

void DWGClarinetAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void DWGClarinetAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (samplesPerBlock);

    // 50 Hz floor covers clarinet's full written range (D3 ≈ 147 Hz) with
    // margin for pitch-bend experiments later; matches the lowestFreq used
    // during Stage 1–2 validation.
    clarinet.prepare (sampleRate, 50.0f);
    clarinet.setReedParameters (reedOffsetParam->get(), reedSlopeParam->get());
    clarinet.setBellReflection (bellReflectParam->get());

    heldNotes.clear();
    currentMidiNote = -1;
}

void DWGClarinetAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool DWGClarinetAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

//==============================================================================
//  MIDI handling — monophonic note stack.
//  Real clarinet: one reed. A new key while one is held retunes the SAME
//  voice (no second oscillator spawned). Releasing falls back to the next
//  held note if any, otherwise the reed stops (noteOff envelope release).
//==============================================================================
void DWGClarinetAudioProcessor::handleNoteOn (int midiNote, float velocity)
{
    // Remove if already in the stack (defensive against duplicate note-ons),
    // then push to the back — back of stack = most recently pressed = the
    // note currently sounding.
    heldNotes.erase (std::remove (heldNotes.begin(), heldNotes.end(), midiNote), heldNotes.end());
    heldNotes.push_back (midiNote);

    currentMidiNote = midiNote;
    clarinet.noteOn (midiNoteToFreq (midiNote), velocity);
}

void DWGClarinetAudioProcessor::handleNoteOff (int midiNote)
{
    heldNotes.erase (std::remove (heldNotes.begin(), heldNotes.end(), midiNote), heldNotes.end());

    if (midiNote != currentMidiNote)
        return; // releasing a note that wasn't the sounding one — no-op

    if (! heldNotes.empty())
    {
        // Fall back to the most recently pressed still-held note, re-trigger
        // its breath envelope at a moderate velocity (legato-style retune
        // would skip the attack; re-triggering is simpler and matches how
        // Stage 1–2's noteOn() is designed to be called).
        int fallbackNote = heldNotes.back();
        currentMidiNote = fallbackNote;
        clarinet.noteOn (midiNoteToFreq (fallbackNote), 0.7f);
    }
    else
    {
        currentMidiNote = -1;
        clarinet.noteOff (0.5f);
    }
}

void DWGClarinetAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Live-update reed/bell parameters every block (cheap, no need to
    // sample-smooth at Stage 1–2 — revisit if clicks appear when twiddling
    // knobs during a sustained note).
    clarinet.setReedParameters (reedOffsetParam->get(), reedSlopeParam->get());
    clarinet.setBellReflection (bellReflectParam->get());

    const int numSamples = buffer.getNumSamples();

    // Walk MIDI events in timestamp order, rendering clarinet samples up to
    // each event, then handling the event, so note changes land on the
    // correct sample within the block.
    int samplePos = 0;
    for (const auto metadata : midiMessages)
    {
        const auto message = metadata.getMessage();
        const int eventSample = metadata.samplePosition;

        // Render up to this event
        for (; samplePos < eventSample && samplePos < numSamples; ++samplePos)
        {
            float y = clarinet.process();
            for (int ch = 0; ch < totalNumOutputChannels; ++ch)
                buffer.setSample (ch, samplePos, y);
        }

        if (message.isNoteOn())
            handleNoteOn (message.getNoteNumber(), message.getFloatVelocity());
        else if (message.isNoteOff())
            handleNoteOff (message.getNoteNumber());
        else if (message.isAllNotesOff() || message.isAllSoundOff())
        {
            heldNotes.clear();
            currentMidiNote = -1;
            clarinet.noteOff (0.0f);
        }
    }

    // Render the remainder of the block after the last MIDI event
    for (; samplePos < numSamples; ++samplePos)
    {
        float y = clarinet.process();
        for (int ch = 0; ch < totalNumOutputChannels; ++ch)
            buffer.setSample (ch, samplePos, y);
    }
}

//==============================================================================
bool DWGClarinetAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* DWGClarinetAudioProcessor::createEditor()
{
    return new DWGClarinetAudioProcessorEditor (*this);
}

//==============================================================================
void DWGClarinetAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::MemoryOutputStream stream (destData, true);
    stream.writeFloat (reedOffsetParam->get());
    stream.writeFloat (reedSlopeParam->get());
    stream.writeFloat (bellReflectParam->get());
}

void DWGClarinetAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    juce::MemoryInputStream stream (data, (size_t) sizeInBytes, false);
    if (stream.getNumBytesRemaining() >= (int) (3 * sizeof (float)))
    {
        *reedOffsetParam  = stream.readFloat();
        *reedSlopeParam   = stream.readFloat();
        *bellReflectParam = stream.readFloat();
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DWGClarinetAudioProcessor();
}
