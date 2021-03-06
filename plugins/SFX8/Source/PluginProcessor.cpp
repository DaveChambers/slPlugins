/*
  ==============================================================================

    This file was auto-generated by the Introjucer!

    It contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Voice.h"

using namespace gin;

static String waveTextFunc (const Parameter&, float v)
{
    switch (int (v))
    {
        case 0: return "Square";
        case 1: return "Saw";
        case 2: return "Sine";
        case 3: return "Noise";
        case 4: return "Triangle";
        case 5: return "Pink";
        case 6: return "Tan";
        case 7: return "Whistle";
        case 8: return "Breaker";
        default:
            jassertfalse;
            return {};
    }
}

//==============================================================================
SFXAudioProcessor::SFXAudioProcessor()
{
    // Add voices
    for (int i = 0; i < 32; i++)
        addVoice (new Voice (*this));

    int notes[] = { 48, 49, 50, 51,
                    44, 45, 46, 47,
                    40, 41, 42, 43,
                    36, 37, 38, 39 };

    // Add pads
    for (int i = 0; i < 16; i++)
    {
        auto p = new Pad (state, i, notes[i]);
        pads.add (p);
        p->name = uniqueName (p->name);
    }
    
    // Add parameters
    for (int i = 0; i < 16; i++)
    {
        SfxrParams p;
        auto ids = p.getParams();
        for (auto id : ids)
        {
            String uniqueId = String (id.c_str()) + String (i + 1);

            addExtParam (uniqueId,
                         String (p.getName (id).c_str()) + " " + String (i + 1),
                         p.getName (id),
                         "",
                         { p.getMin (id), p.getMax (id), 0.0f, 1.0f },
                         p.getDefault (id),
                         0.0f,
                         id == "waveType" ? waveTextFunc : nullptr);
        }
        
        for (auto id : ids)
        {
            String uniqueId = String (id.c_str()) + String (i + 1) + "l";
            
            addExtParam (uniqueId,
                         String (p.getName (id).c_str()) + " " + String (i + 1) + " Lock",
                         p.getName (id) + " Lock",
                         "",
                         { 0.0f, 1.0f, 1.0f, 1.0f },
                         0.0f,
                         0.0f);
        }
    }

    // Add parameters to pads
    auto allParams = getPluginParameters();
    int paramsPerPad = allParams.size() / 16;

    for (int i = 0; i < 16; i++)
    {
        auto p = pads[i];

        for (int j = 0; j < paramsPerPad / 2; j++)
            p->pluginParams.add (allParams[i * paramsPerPad + j]);
        
        for (int j = paramsPerPad / 2; j < paramsPerPad; j++)
            p->pluginLockParams.add (allParams[i * paramsPerPad + j]);
        
        p->toPluginParams();
    }
}

SFXAudioProcessor::~SFXAudioProcessor()
{
}

//==============================================================================
void SFXAudioProcessor::stateUpdated()
{
    for (int i = 0; i < pads.size(); i++)
    {
        auto p = pads[i];
        
        auto nameKey = "name" + String (i);
        auto noteKey = "note" + String (i);
        
        if (state.hasProperty (nameKey))
            p->name = state.getProperty (nameKey);
        if (state.hasProperty (noteKey))
            p->note = state.getProperty (noteKey);
    }
}

void SFXAudioProcessor::updateState()
{
    for (int i = 0; i < pads.size(); i++)
    {
        auto p = pads[i];
        
        state.setProperty ("name" + String (i), p->name.get(), nullptr);
        state.setProperty ("note" + String (i), p->note.get(), nullptr);
    }
}

//==============================================================================
void SFXAudioProcessor::midiNoteOn (int note, int velocity)
{
    ScopedLock sl (lock);

    if (! midiLearn && note > 0)
        userMidi.addEvent (MidiMessage::noteOn (1, note, uint8 (velocity)), 0);
}

void SFXAudioProcessor::midiNoteOff (int note, int velocity)
{
    ScopedLock sl (lock);

    if (! midiLearn && note > 0)
        userMidi.addEvent (MidiMessage::noteOff (1, note, uint8 (velocity)), 0);
}

//==============================================================================
void SFXAudioProcessor::prepareToPlay (double sampleRate_, int /*samplesPerBlock*/)
{
    setCurrentPlaybackSampleRate (sampleRate_);
}

void SFXAudioProcessor::releaseResources()
{
}

void SFXAudioProcessor::trackMidi (MidiBuffer& midi, int numSamples)
{
    // fade out old messages
    double t = numSamples / gin::GinProcessor::getSampleRate() * 1000;
    for (auto& c : midiCnt)
        c = jmax (0, int (c - t));

    // track new messages
    int pos = 0;
    MidiMessage msg;
    auto itr = MidiBuffer::Iterator (midi);

    while (itr.getNextEvent (msg, pos))
    {
        int n = msg.getNoteNumber();
        if (msg.isNoteOn())
        {
            midiOn[n]++;
            midiCnt[n] = 100;
            
            if (midiLearn)
            {
                MessageManager::getInstance()->callAsync ([this, n]
                                                          {
                                                              for (auto p : pads)
                                                                  if (p->note == n)
                                                                      p->note = -1;
                                                              
                                                              if (auto p = pads[currentPad])
                                                                  p->note = n;

                                                              delayedLambda ([this]
                                                                             {
                                                                                 if (currentPad < 15)
                                                                                 {
                                                                                     currentPad++;
                                                                                     if (onCurrentPageChanged)
                                                                                         onCurrentPageChanged();
                                                                                 }
                                                                                 else
                                                                                 {
                                                                                     midiLearn = false;
                                                                                 }
                                                                             }, 500);
                                                          });
            }
        }
        else if (msg.isNoteOff())
        {
            midiOn[n]--;
            if (midiOn[n] < 0)
                midiOn[n] = 0;
        }
        else if (msg.isAllNotesOff() || msg.isAllSoundOff())
        {
            memset (midiOn, 0, sizeof (midiOn));
            memset (midiCnt, 0, sizeof (midiCnt));
        }
    }
}

void SFXAudioProcessor::processBlock (AudioSampleBuffer& buffer, MidiBuffer& midi)
{
    ScopedLock sl (lock);
    ScopedNoDenormals noDenormals;

    if (userMidi.getNumEvents() > 0)
    {
        userMidi.addEvents (midi, 0, buffer.getNumSamples(), 0);

        trackMidi (userMidi, buffer.getNumSamples());
        renderNextBlock (buffer, userMidi, 0, buffer.getNumSamples());
        userMidi.clear();
    }
    else
    {
        trackMidi (midi, buffer.getNumSamples());
        renderNextBlock (buffer, midi, 0, buffer.getNumSamples());
    }
}

String SFXAudioProcessor::uniqueName (String prefix)
{
    int count = state.getProperty ("count" + prefix, 1);
    state.setProperty ("count" + prefix, count + 1, nullptr);
    
    return prefix + " " + String (count);
}

//==============================================================================
bool SFXAudioProcessor::hasEditor() const
{
    return true;
}

AudioProcessorEditor* SFXAudioProcessor::createEditor()
{
    editor = new SFXAudioProcessorEditor (*this);
    return editor;
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SFXAudioProcessor();
}
