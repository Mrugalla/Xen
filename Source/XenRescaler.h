#pragma once
#include "MPESplit.h"
#include "Math.h"
#include <functional>

namespace xen
{
	using MidiBuffer = juce::MidiBuffer;
	using MidiMessage = juce::MidiMessage;
	using Unit8 = juce::uint8;

	class XenRescaler
	{
		static constexpr double PBRange = 16383.;
		static constexpr double PBRangeHalf = PBRange / 2.;
	public:
		XenRescaler() :
			curNote(MidiMessage::noteOn(1, 0, Unit8(0))),
			pbRange(0.)
		{
		}

		void update(double _pbRange) noexcept
		{
			pbRange = _pbRange;
		}

		void operator()(MidiBuffer& midi, MidiBuffer& buffer,
			const double* freqTable)
		{
			for (const auto it : midi)
			{
				const auto ts = it.samplePosition;
				const auto msg = it.getMessage();
				if (msg.isNoteOn())
				{
					const auto channel = msg.getChannel();
					const auto velo = msg.getFloatVelocity();
					const auto pitch = msg.getNoteNumber();
					const auto freq = freqTable[pitch];
					processNoteOn(buffer, velo, freq, pbRange, channel, ts);
				}
				else if (msg.isNoteOff())
					processNoteOff(buffer, ts);
				else
					buffer.addEvent(msg, ts);
			}
		}
	
	private:
		MidiMessage curNote;
		double pbRange;

		void processNoteOn(MidiBuffer& buffer, float velocity,
			double freq, double pitchbendRange, int channel, int ts)
		{
			curNote.setChannel(channel);
			curNote.setVelocity(velocity);

			const auto note = math::freqToNote(freq);
			const auto noteRound = std::round(note);
			curNote.setNoteNumber(static_cast<int>(noteRound));

			const auto noteFrac = (note - noteRound) / pitchbendRange;
			const auto pitchbend = noteFrac * PBRangeHalf + PBRangeHalf;

			buffer.addEvent(MidiMessage::pitchWheel(channel, static_cast<int>(pitchbend)), ts);
			buffer.addEvent(curNote, ts);
		}

		void processNoteOff(MidiBuffer& buffer, int ts)
		{
			const auto channel = curNote.getChannel();
			const auto note = curNote.getNoteNumber();
			buffer.addEvent(MidiMessage::noteOff(channel, note), ts);
		}
	};

	struct XenRescalerMPE
	{
		using MPE = mpe::Split;

		XenRescalerMPE(MPE& _mpe) :
			buffer(),
			xenRescaler(),
			mpe(_mpe)
		{
		}

		void update(double pbRange) noexcept
		{
			for (auto& voice : xenRescaler)
				voice.update(pbRange);
		}

		void operator()(MidiBuffer& midiMessages,
			const double* freqTable, int numSamples)
		{
			buffer.clear();
			for (auto ch = 0; ch < mpe::NumChannelsMPE; ++ch)
			{
				auto& rescaler = xenRescaler[ch];
				auto& midi = mpe[ch + 2];
				rescaler(midi, buffer, freqTable);
			}
			midiMessages.addEvents(buffer, 0, numSamples, 0);
		}

	private:
		MidiBuffer buffer;
		std::array<XenRescaler, mpe::NumChannelsMPE> xenRescaler;
		MPE& mpe;
	};
}