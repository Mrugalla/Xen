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
			curNote(MidiMessage::noteOn(1, 0, Unit8(0)))
		{
		}

		void operator()(MidiBuffer& midi, MidiBuffer& buffer,
			double xen, double anchorPitch, double anchorFreq,
			double pitchbendRange, bool stepsIn12)
		{
			const auto func = stepsIn12 ? [](double pitch,
				double xen, double anchorPitch, double anchorFreq,
				double pitchbendRange, int ts)
				{
					return math::noteToFreqIn12Steps(pitch, xen, anchorPitch, anchorFreq);
				} : [](double pitch,
					double xen, double anchorPitch, double anchorFreq,
					double pitchbendRange, int ts)
				{
					return math::noteToFreq(pitch, xen, anchorPitch, anchorFreq);
				};

			for (const auto it : midi)
			{
				const auto ts = it.samplePosition;
				const auto msg = it.getMessage();
				if (msg.isNoteOn())
				{
					const auto channel = msg.getChannel();
					const auto velo = msg.getFloatVelocity();
					const auto pitch = static_cast<double>(msg.getNoteNumber());
					const auto freq = func(pitch, xen, anchorPitch, anchorFreq, pitchbendRange, ts);
					processNoteOn(buffer, velo, freq, pitchbendRange, channel, ts);
				}
				else if (msg.isNoteOff())
					processNoteOff(buffer, ts);
				else
					buffer.addEvent(msg, ts);
			}
		}
	
	private:
		MidiMessage curNote;

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
		using MPE = mpe::MPESplit;

		XenRescalerMPE(MPE& _mpe) :
			buffer(),
			xenRescaler(),
			mpe(_mpe)
		{
		}

		void operator()(MidiBuffer& midiMessages,
			double xen, double anchorPitch, double anchorFreq,
			double pitchbendRange, int numSamples, bool stepsIn12)
		{
			buffer.clear();
			for (auto ch = 0; ch < mpe::NumChannelsMPE; ++ch)
			{
				auto& rescaler = xenRescaler[ch];
				auto& midi = mpe[ch + 2];

				rescaler(midi, buffer, xen, anchorPitch, anchorFreq, pitchbendRange, stepsIn12);
			}
			midiMessages.addEvents(buffer, 0, numSamples, 0);
		}

	private:
		MidiBuffer buffer;
		std::array<XenRescaler, mpe::NumChannelsMPE> xenRescaler;
		MPE& mpe;
	};
}