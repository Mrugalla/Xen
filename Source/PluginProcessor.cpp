#include "PluginProcessor.h"

static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    const auto valToStrBool = [](bool e, int)
        {
            return juce::String(e ? "Enabled" : "Disabled");
        };
    const auto strToValBool = [](const juce::String& s)
        {
            return s.getIntValue() != 0;
        };

    const auto valToStrPlayMode = [](bool e, int)
    {
        return e ? juce::String("Nearest") : juce::String("Rescale");
    };

    const auto valToStrPitch = [](int note, int)
	{
            return juce::MidiMessage::getMidiNoteName(note, true, true, 3) + " [" + juce::String(note) + "]";
	};

    const auto valToStrHzInt = [](int hz, int)
	{
		return juce::String(hz) + "hz";
	};

    const auto valToStrXen = [](int xen, int)
    {
		return juce::String(xen) + " edo";
	};

    const auto valToStrSemi = [](int semi, int)
    {
        return juce::String(semi) + " semi";
    };

    params.push_back(std::make_unique<juce::AudioParameterInt>
    (
        "xen", "Xen", 4, 48, 12, "", valToStrXen
    ));
    params.push_back(std::make_unique<juce::AudioParameterInt>
    (
        "basepitch", "Base Pitch", 0, 127, 69, "", valToStrPitch
    ));
    params.push_back(std::make_unique<juce::AudioParameterInt>
    (
		"mastertune", "Master Tune", 420, 460, 440, "", valToStrHzInt
	));
    params.push_back(std::make_unique<juce::AudioParameterInt>
    (
        "pbrange", "Pitchbend Range", 1, 48, 48, "", valToStrSemi
    ));
    params.push_back(std::make_unique<juce::AudioParameterBool>
    (
        "autompe", "Auto MPE", true
    ));
    params.push_back(std::make_unique<juce::AudioParameterBool>
    (
        "playmode", "Play Mode", false, "", valToStrPlayMode
    ));
    params.push_back(std::make_unique<juce::AudioParameterBool>
    (
        "usesynth", "Use Synth", true
    ));

    return { params.begin(), params.end() };
}

XenAudioProcessor::XenAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if !JucePlugin_IsMidiEffect
                      #if !JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
    apvts(*this, nullptr, JucePlugin_Name, createParameterLayout()),
    xen(*apvts.getParameter("xen")),
    basePitch(*apvts.getParameter("basepitch")),
    masterTune(*apvts.getParameter("mastertune")),
    pbRange(*apvts.getParameter("pbrange")),
    autoMPE(*apvts.getParameter("autompe")),
    playMode(*apvts.getParameter("playmode")),
    useSynth(*apvts.getParameter("usesynth")),

    autoMPEProcessor(),
    mpeSplit(),
    synth(mpeSplit),
    xenRescaler(mpeSplit)
#endif
{
}

XenAudioProcessor::~XenAudioProcessor()
{
}

//==============================================================================
const juce::String XenAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool XenAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool XenAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool XenAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double XenAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int XenAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int XenAudioProcessor::getCurrentProgram()
{
    return 0;
}

void XenAudioProcessor::setCurrentProgram(int)
{
}

const juce::String XenAudioProcessor::getProgramName(int)
{
    return {};
}

void XenAudioProcessor::changeProgramName(int, const juce::String&)
{
}

void XenAudioProcessor::prepareToPlay(double sampleRate, int)
{
    synth.prepare(sampleRate);
}

void XenAudioProcessor::releaseResources()
{
}

bool XenAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto mono = juce::AudioChannelSet::mono();
    const auto stereo = juce::AudioChannelSet::stereo();
    const auto mainOut = layouts.getMainOutputChannelSet();
    
    if (mainOut != mono && mainOut != stereo)
		return false;

#if !JucePlugin_IsSynth
    const auto mainIn = layouts.getMainInputChannelSet();
    if (mainIn != mono && mainIn != stereo)
        return false;
#endif

    return true;
}

void XenAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    const juce::ScopedNoDenormals noDenormals;

    const auto numSamples = buffer.getNumSamples();
    if (numSamples == 0)
        return;
    auto samples = buffer.getArrayOfWritePointers();
    const auto numChannels = buffer.getNumChannels();
    for (auto ch = 0; ch < numChannels; ++ch)
        juce::FloatVectorOperations::clear(samples[ch], numSamples);

    const auto useSynthVal = useSynth.getValue() > .5f;
    const auto playModeVal = playMode.getValue() > .5f;
    const auto autoMPEVal = autoMPE.getValue() > .5f;
    const auto pbRangeVal = static_cast<double>(pbRange.convertFrom0to1(pbRange.getValue()));
    
    const auto xenVal = static_cast<double>(xen.convertFrom0to1(xen.getValue()));
    const auto basePitchVal = static_cast<double>(basePitch.convertFrom0to1(basePitch.getValue()));
    const auto masterTuneVal = static_cast<double>(masterTune.convertFrom0to1(masterTune.getValue()));

    if (autoMPEVal)
        autoMPEProcessor(midiMessages);

    mpeSplit(midiMessages);

    if (useSynthVal)
        synth(samples, xenVal, basePitchVal, masterTuneVal, numChannels, numSamples, playModeVal);
    
    const auto type = playModeVal ? xen::XenRescalerMPE::Type::Nearest : xen::XenRescalerMPE::Type::Rescale;
    xenRescaler(midiMessages, xenVal, basePitchVal, masterTuneVal, pbRangeVal, numSamples, type);
}

bool XenAudioProcessor::hasEditor() const
{
    return false;
}
juce::AudioProcessorEditor* XenAudioProcessor::createEditor()
{
    return nullptr;
}

void XenAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}
void XenAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new XenAudioProcessor();
}
