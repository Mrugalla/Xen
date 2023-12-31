#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>

namespace mpe
{
	using MidiBuffer = juce::MidiBuffer;
	using MidiMessage = juce::MidiMessage;
	static constexpr int NumChannels = 16;
	static constexpr int NumChannelsMPE = NumChannels - 1;
	enum { kSysex };
}