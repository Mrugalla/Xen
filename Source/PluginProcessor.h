#pragma once
#define oopsie(x) jassert(!(x))

#include <JuceHeader.h>
#include "AutoMPE.h"
#include "Xen.h"

struct XenAudioProcessor : public juce::AudioProcessor
                            #if JucePlugin_Enable_ARA
                             , public juce::AudioProcessorARAExtension
                            #endif
{
    XenAudioProcessor();
    ~XenAudioProcessor() override;
    void prepareToPlay (double, int) override;
    void releaseResources() override;
   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout&) const override;
   #endif
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;
    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int) override;
    const juce::String getProgramName (int) override;
    void changeProgramName (int, const juce::String&) override;
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameters();

    juce::AudioProcessorValueTreeState apvts;
    juce::RangedAudioParameter &xenSnap, &xen, &anchorFreq, &stepsIn12, &mode, &pbRange;
    mpe::AutoMPE autoMPEProcessor;
    mpe::Split mpeSplit;
    xen::Xen xenProcessor;
    bool xenSnapped;
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
    serum(2)    >> works
    karp        >> works
    surge       >> works

Tested in DAWs:
    cubase      >> works, but timing issues with MPE
    reaper      >> ?
    studio one  >> ?
    ableton     >> ?
    fl studio   >> works :)
    bitwig      >> only mts-esp
    logic       >> works as audio unit

How to use in different DAWs:
    MTS-ESP:
        1. Init this on new track
        2. Use MTS-ESP mode
        3. Use MTS-ESP clients like Surge XT or Serum 2
    Cubase MPE:
        Make instrument track: Xen.
        Make instrument track: Target Synth (Vital/Serum/whatever)
        MIDI input on Target Synth: Xen's MIDI Output.
        Set MIDI Channels to 'all'.
    FL MPE:
        Click the cogwheel icon in the xen plugin bar.
        Open the 2nd tab (its called vst wrapper settings on hover)
        there choose a midi output port.
        Same procedure for the receiving plugin,
        just use the chosen port for input in the same menu
    Reaper MPE:
	    Place Xen before instrument in plugin chain.
    Ableton MPE:
	    Check in pivo guide: https://www.dmitrivolkov.com/projects/pivotuner/Pivotuner_Guide.pdf p7
*/