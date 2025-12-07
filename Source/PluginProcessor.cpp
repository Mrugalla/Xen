#include "PluginProcessor.h"
#include "Axiom.h"
#include "Range.h"

juce::AudioProcessorValueTreeState::ParameterLayout XenAudioProcessor::createParameters()
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

    const auto valToStrXen = [&](float xen, int)
    {
		const auto xFloor = std::floor(xen);
        const auto xFrac = xen - xFloor;
        if (xFrac == 0.f)
            return juce::String(static_cast<int>(xFloor)) + " tet";
        return juce::String(xen, 2) + " tet";
	};

    const auto valToStrSemi = [](int semi, int)
    {
        return juce::String(semi) + " semi";
    };

    const auto atr = juce::AudioParameterBoolAttributes();
    params.push_back(std::make_unique<juce::AudioParameterBool>
    (
        "xensnap", "Xen Snap", true, atr
    ));
	const auto atrXen = juce::AudioParameterFloatAttributes().withStringFromValueFunction(valToStrXen);
	const auto xenMinF = static_cast<float>(axiom::MinXen);
	const auto xenMaxF = static_cast<float>(axiom::MaxXen);
	const auto xenRange = xenMaxF - xenMinF;
    const auto xenNormRange = juce::NormalisableRange<float>
    (
		xenMinF, xenMaxF,
        [xenRange](float start, float, float x)
        {
            return start + xenRange * x;
        },
        [xenRange](float start, float, float x)
        {
            return (x - start) / xenRange;
        },
        [&](float start, float end, float x)
        {
			return juce::jlimit(start, end, xenSnapped ? std::round(x) : x);
        }
	);
    params.push_back(std::make_unique<juce::AudioParameterFloat>
    (
        "xen", "Xen", xenNormRange, 12.f, atrXen
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
	
    params.push_back(std::make_unique<juce::AudioParameterBool>
    (
        "stepsIn12", "Steps In 12", false, atr
    ));
    const auto atrMode = juce::AudioParameterBoolAttributes().withStringFromValueFunction(valToStrMode);
    params.push_back(std::make_unique<juce::AudioParameterBool>
    (
        "mode", "Mode", true, atrMode
    ));
    const auto atrPBRange = juce::AudioParameterIntAttributes().withStringFromValueFunction(valToStrSemi);
    params.push_back(std::make_unique<juce::AudioParameterInt>
    (
        "pbrange", "Pitchbend Range", 1, 48, 48, atrPBRange
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
    apvts(*this, nullptr, JucePlugin_Name, createParameters()),
	xenSnap(*apvts.getParameter("xensnap")),
    xen(*apvts.getParameter("xen")),
    anchorFreq(*apvts.getParameter("anchorfreq")),
    stepsIn12(*apvts.getParameter("stepsIn12")),
    mode(*apvts.getParameter("mode")),
    pbRange(*apvts.getParameter("pbrange")),
    autoMPEProcessor(),
    mpeSplit(),
    xenProcessor(mpeSplit),
    xenSnapped(true)
#endif
{
    
}

XenAudioProcessor::~XenAudioProcessor()
{
    
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
    xenProcessor.prepare(sampleRate);
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

void XenAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    const juce::ScopedNoDenormals noDenormals;
    const auto numSamples = buffer.getNumSamples();
    if (numSamples == 0)
        return;
    {
		const auto totalNumOutputChannels = getTotalNumOutputChannels();
		const auto totalNumInputChannels = getTotalNumInputChannels();
		for (auto i = totalNumOutputChannels; i < totalNumInputChannels; ++i)
			buffer.clear(i, 0, numSamples);
    }
    autoMPEProcessor(midi);
    mpeSplit(midi);

    xenSnapped = xenSnap.getValue() > .5f;
	const auto xenV = xen.convertFrom0to1(xen.getValue());
	const auto anchorFreqV = math::noteToFreq(anchorFreq.convertFrom0to1(anchorFreq.getValue()));
	const auto pbRangeV = pbRange.convertFrom0to1(pbRange.getValue());
	const auto mtsEnabledV = mode.getValue() > .5f;
	const auto stepsIn12V = stepsIn12.getValue() > .5f;
    xenProcessor.updateParameters(xenV, anchorFreqV, pbRangeV, mtsEnabledV, stepsIn12V);

	auto samples = buffer.getArrayOfWritePointers();
    xenProcessor(samples, midi, numSamples);
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
