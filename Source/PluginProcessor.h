#pragma once
#define oopsie(condition) jassert(!(condition))

#include <JuceHeader.h>
#include "MPESplit.h"
#include "AutoMPE.h"
#include "Synth.h"
#include "XenRescaler.h"
#include "NoteDelay.h"
#include "MTSXen.h"

class XenAudioProcessor : public juce::AudioProcessor
                            #if JucePlugin_Enable_ARA
                             , public juce::AudioProcessorARAExtension
                            #endif
{
public:
    //==============================================================================
    XenAudioProcessor();
    ~XenAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double, int) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout&) const override;
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
    void setCurrentProgram (int) override;
    const juce::String getProgramName (int) override;
    void changeProgramName (int, const juce::String&) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;


    juce::AudioProcessorValueTreeState apvts;
    juce::RangedAudioParameter &mode, &xen, &basePitch, &masterTune, &pbRange, &playMode, &useSynth;

    mpe::AutoMPE autoMPEProcessor;
    mpe::MPESplit mpeSplit;

    mts::Xen mtsXen;

    syn::Synth synth;
    xen::XenRescalerMPE xenRescaler;
    bool mtsEnabled;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (XenAudioProcessor)
};

/*
--- todo ---

bugs:

parameters:

features:

workflow:
    make GUI

Tested in Synths:
    vital       >> works, but on session reopen doesn't remember mpe enabled despite showing it as enabled
    serum       >> works
    karp        >> works
    surge       >> works

Tested in DAWs:
    cubase      >> works, but timing issues
    reaper      >> ?
    studio one  >> ?
    ableton     >> ?
    fl studio   >> works :)
    bitwig      >> only via mts-esp
    logic       >> works as audio unit

How to use in different DAWs:
    Cubase:
        Make instrument track: Xen.
        Make instrument track: Target Synth (Vital/Serum/whatever)
        MIDI input on Target Synth: Xen's MIDI Output.
        Set MIDI Channels to 'all'.
    FL:
        Click the cogwheel icon in the xen plugin bar.
        Open the 2nd tab (its called vst wrapper settings on hover)
        there choose a midi output port.
        Same procedure for the receiving plugin,
        just use the chosen port for input in the same menu
    Reaper:
	    Place Xen before instrument in plugin chain.
    Ableton:
	    Check in pivo guide: https://www.dmitrivolkov.com/projects/pivotuner/Pivotuner_Guide.pdf p7
    Bitwig:
        1. Init this on some track
        2. Use MTS-ESP mode
*/