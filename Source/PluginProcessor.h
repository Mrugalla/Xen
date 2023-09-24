#pragma once

#define oopsie(condition) jassert(!(condition))

#include <JuceHeader.h>
#include "MPESplit.h"
#include "AutoMPE.h"
#include "Synth.h"
#include "XenRescaler.h"
#include "NoteDelay.h"

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

    juce::AudioProcessorValueTreeState apvts;
    juce::RangedAudioParameter &xen, &basePitch, &masterTune, &pbRange, &autoMPE, &playMode, &useSynth;

    mpe::AutoMPE autoMPEProcessor;
    mpe::MPESplit mpeSplit;
    syn::XenSynthMPE synth;
    xen::XenRescalerMPE xenRescaler;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (XenAudioProcessor)
};

/*
--- debug environments ---

D:\PluginDevelopment\JUCE\extras\AudioPluginHost\Builds\VisualStudio2022\x64\Release\App\AudioPluginHost.exe
C:\Program Files\Steinberg\Cubase 9.5\Cubase9.5.exe

--- todo ---

bugs:
	mild timing problems in cubase

parameters:
    remove master tune?

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
    bitwig      >> only allows note expression via clap :<
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
        with MTS-ESP maybe.
*/