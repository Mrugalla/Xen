#pragma once
#include "MPEUtils.h"

namespace mpe
{
	struct MPESplit
	{
		static constexpr int Size = NumChannels + 1;
		using Buffers = std::array<MidiBuffer, Size>;

		MPESplit() :
			buffers()
		{
		}

		void operator()(MidiBuffer& midiIn)
		{
			for (auto& buffer : buffers)
				buffer.clear();

			for (const auto it : midiIn)
			{
				const auto msg = it.getMessage();
				const auto ch = msg.getChannel();
				buffers[ch].addEvent(msg, it.samplePosition);
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
		Buffers buffers;
	};
}