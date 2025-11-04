#include "PluginProcessor.h"
#include "Axiom.h"
#include "Range.h"

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

    const auto valToStrMode = [](bool e, int)
    {
        return juce::String(e ? "MTS-ESP" : "MPE");
    };

    const auto valToStrPitch = [](int note, int)
	{
        return juce::MidiMessage::getMidiNoteName(note, true, true, 3) + " [" + juce::String(note) + "]";
	};

    const auto valToStrHzInt = [](int pitch, int)
	{
		const auto pitchD = static_cast<double>(pitch);
		const auto hz = math::noteToFreq(pitchD);
		return juce::String(hz) + "hz";
	};

    const auto strToValHzInt = [](const juce::String& s)
        {
            auto valHz = static_cast<double>(s.getFloatValue());
            auto valPitch = static_cast<int>(std::round(math::freqToNote(valHz)));
            return valPitch;
        };

    const auto valToStrXen = [](int xen, int)
    {
		return juce::String(xen) + " edo";
	};

    const auto valToStrSemi = [](int semi, int)
    {
        return juce::String(semi) + " semi";
    };

    const auto atr = juce::AudioParameterBoolAttributes();

    params.push_back(std::make_unique<juce::AudioParameterBool>
    (
        "mode", "Mode", true, atr
    ));
	const auto atrXen = juce::AudioParameterIntAttributes().withStringFromValueFunction(valToStrXen);
    params.push_back(std::make_unique<juce::AudioParameterInt>
    (
        "xen", "Xen", axiom::MinXen, axiom::MaxXen, 12, atrXen
    ));
	const auto atrAnchorPitch = juce::AudioParameterIntAttributes().withStringFromValueFunction(valToStrPitch);
    params.push_back(std::make_unique<juce::AudioParameterInt>
    (
        "anchorpitch", "Anchor Pitch", 0, 127, 69, atrAnchorPitch
    ));
    const auto pitchMin = static_cast<int>(std::round(math::freqToNote(20.)));
    const auto pitchMax = static_cast<int>(std::round(math::freqToNote(20000.)));
	const auto pitchDefault = static_cast<int>(std::round(math::freqToNote(440.)));
	const auto atrAnchorFreq = juce::AudioParameterIntAttributes()
        .withStringFromValueFunction(valToStrHzInt)
	    .withValueFromStringFunction(strToValHzInt);
    params.push_back(std::make_unique<juce::AudioParameterInt>
    (
		"anchorfreq", "Anchor Freq", pitchMin, pitchMax, pitchDefault, atrAnchorFreq
	));
	const auto atrPBRange = juce::AudioParameterIntAttributes().withStringFromValueFunction(valToStrSemi);
    params.push_back(std::make_unique<juce::AudioParameterInt>
    (
        "pbrange", "Pitchbend Range", 1, 48, 48, atrPBRange
    ));
    params.push_back(std::make_unique<juce::AudioParameterBool>
    (
        "stepsIn12", "Steps In 12", false, atr
    ));
    params.push_back(std::make_unique<juce::AudioParameterBool>
    (
        "usesynth", "Use Synth", true, atr
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
	mode(*apvts.getParameter("mode")),
    xen(*apvts.getParameter("xen")),
    basePitch(*apvts.getParameter("anchorpitch")),
    masterTune(*apvts.getParameter("anchorfreq")),
    pbRange(*apvts.getParameter("pbrange")),
    playMode(*apvts.getParameter("stepsIn12")),
    useSynth(*apvts.getParameter("usesynth")),

    autoMPEProcessor(),
    mpeSplit(),
    mtsXen(),
    synth(mpeSplit),
    xenRescaler(mpeSplit),
    mtsEnabled(false)
#endif
{
    if(MTS_CanRegisterMaster())
		MTS_RegisterMaster();
}

XenAudioProcessor::~XenAudioProcessor()
{
    MTS_DeregisterMaster();
}

const juce::String XenAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool XenAudioProcessor::acceptsMidi() const
{
    return true;
}

bool XenAudioProcessor::producesMidi() const
{
    return true;
}

bool XenAudioProcessor::isMidiEffect() const
{
    return false;
}

double XenAudioProcessor::getTailLengthSeconds() const
{
    return 0.;
}

int XenAudioProcessor::getNumPrograms()
{
    return 1;
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
    const auto stereo = juce::AudioChannelSet::stereo();
    const auto mainOut = layouts.getMainOutputChannelSet();
    if (mainOut != stereo)
		return false;
    return true;
}

void XenAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    const juce::ScopedNoDenormals noDenormals;

    const auto numSamples = buffer.getNumSamples();
    if (numSamples == 0)
        return;
    auto samples = buffer.getArrayOfWritePointers();
    {
		const auto totalNumOutputChannels = getTotalNumOutputChannels();
		const auto totalNumInputChannels = getTotalNumInputChannels();
		for (auto i = totalNumOutputChannels; i < totalNumInputChannels; ++i)
			buffer.clear(i, 0, numSamples);
    }
    autoMPEProcessor(midiMessages);
    mpeSplit(midiMessages);

    const auto playSynth = useSynth.getValue() > .5f;
    const auto stepsIn12 = playMode.getValue() > .5f;
    const auto xenVal = static_cast<double>(xen.convertFrom0to1(xen.getValue()));
    const auto basePitchVal = static_cast<double>(basePitch.convertFrom0to1(basePitch.getValue()));
    const auto masterTuneVal = static_cast<double>(math::noteToFreq(masterTune.convertFrom0to1(masterTune.getValue())));

    const bool _mtsEnabled = mode.getValue() > .5f;
    if (_mtsEnabled)
    {
        if (!mtsEnabled)
        {
            mtsEnabled = true;
            mtsXen.reset();
        }
        mtsXen(xenVal, basePitchVal, masterTuneVal, stepsIn12);
        if (playSynth)
            synth.synthMTS(samples, numSamples);
        return;
    }
    // MPE processing:
    mtsEnabled = false;
    if (playSynth)
    {
		synth.updateParameters(xenVal, basePitchVal, masterTuneVal, stepsIn12);
        synth.synthMPE(samples, numSamples);
    }
    const auto pbRangeVal = static_cast<double>(pbRange.convertFrom0to1(pbRange.getValue()));
    xenRescaler(midiMessages, xenVal, basePitchVal, masterTuneVal, pbRangeVal, numSamples, stepsIn12);
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
