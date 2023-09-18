#pragma once
#include "MPEUtils.h"

namespace mpe
{
	struct MPESplit
	{
		MPESplit() :
			buffers()
		{
			for (auto& buffer : buffers)
				buffer.ensureSize(1024);
		}

		void operator()(MidiBuffer& midiIn)
		{
			for (auto& buffer : buffers)
				buffer.clear();

			for (const auto midi : midiIn)
			{
				const auto msg = midi.getMessage();
				const auto ch = msg.getChannel();
				buffers[ch].addEvent(msg, midi.samplePosition);
			}

			midiIn.swapWith(buffers[kSysex]);
		}

		MidiBuffer& operator[](int ch) noexcept
		{
			return buffers[ch];
		}

		const MidiBuffer& operator[](int ch) const noexcept
		{
			return buffers[ch];
		}

	protected:
		MidiBuffers buffers;
	};
}