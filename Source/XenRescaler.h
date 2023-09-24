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
		using NoteOnFunc = std::function<void(const MidiMessage&, MidiBuffer&,
			double, double, double, double, int)>;
		using NoteOnFuncs = std::array<NoteOnFunc, 2>;

		static constexpr double PBRange = 16383.;
		static constexpr double PBRangeHalf = PBRange / 2.;
	
	public:
		enum class Type
		{
			Rescale,
			Nearest
		};

		XenRescaler() :
			curNote(MidiMessage::noteOn(1, 0, Unit8(0))),
			noteOnFuncs()
		{
			noteOnFuncs[static_cast<int>(Type::Rescale)] = [&](const MidiMessage& msg, MidiBuffer& buffer,
				double xen, double basePitch, double masterTune,
				double pitchbendRange, int ts)
			{
				const auto channel = msg.getChannel();
				const auto noteNumber = static_cast<double>(msg.getNoteNumber());
				const auto freq = math::noteToFreq(noteNumber, xen, basePitch, masterTune);
				processNoteOn(buffer, msg.getFloatVelocity(), freq, pitchbendRange, channel, ts);
			};

			noteOnFuncs[static_cast<int>(Type::Nearest)] = [&](const MidiMessage& msg, MidiBuffer& buffer,
				double xen, double basePitch, double masterTune,
				double pitchbendRange, int ts)
			{
				const auto channel = msg.getChannel();
				const auto noteNumber = static_cast<double>(msg.getNoteNumber());
				const auto freq = math::noteToFreq(noteNumber);
				const auto cFreq = math::closestFreq(freq, xen, basePitch, masterTune);
				processNoteOn(buffer, msg.getFloatVelocity(), cFreq, pitchbendRange, channel, ts);
			};
		}

		void operator()(MidiBuffer& midi, MidiBuffer& buffer,
			double xen, double basePitch, double masterTune,
			double pitchbendRange, Type type)
		{
			for (const auto it : midi)
			{
				const auto ts = it.samplePosition;
				const auto msg = it.getMessage();
				if (msg.isNoteOn())
				{
					const auto& noteOnFunc = noteOnFuncs[static_cast<int>(type)];
					noteOnFunc(msg, buffer, xen, basePitch, masterTune, pitchbendRange, ts);
				}
				else if (msg.isNoteOff())
					processNoteOff(buffer, ts);
				else
					buffer.addEvent(msg, ts);
			}
		}
	
	private:
		MidiMessage curNote;
		NoteOnFuncs noteOnFuncs;

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
		using Type = XenRescaler::Type;

		XenRescalerMPE(MPE& _mpe) :
			buffer(),
			xenRescaler(),
			mpe(_mpe)
		{
		}

		void operator()(MidiBuffer& midiMessages,
			double xen, double basePitch, double masterTune,
			double pitchbendRange, int numSamples, Type type)
		{
			buffer.clear();

			for (auto ch = 0; ch < mpe::NumChannelsMPE; ++ch)
			{
				auto& rescaler = xenRescaler[ch];
				auto& midi = mpe[ch + 2];

				rescaler(midi, buffer, xen, basePitch, masterTune, pitchbendRange, type);
			}

			midiMessages.addEvents(buffer, 0, numSamples, 0);
		}

	private:
		MidiBuffer buffer;
		std::array<XenRescaler, mpe::NumChannelsMPE> xenRescaler;
		MPE& mpe;
	};
}