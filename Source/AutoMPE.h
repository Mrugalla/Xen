#pragma once
#include "MPEUtils.h"

namespace mpe
{
	struct AutoMPE
	{
		struct Voice
		{
			Voice() :
				note(-1),
				channel(0),
				noteOn(false)
			{}

			int note, channel;
			bool noteOn;
		};

		using Voices = std::array<Voice, NumChannelsMPE>;

		AutoMPE() :
			buffer(),
			voices(),
			channelIdx(0)
		{
		}

		void operator()(MidiBuffer& midiMessages)
		{
			buffer.clear();
			for(const auto it : midiMessages)
			{
				auto msg = it.getMessage();
				
				if (msg.isNoteOn())
					processNoteOn(msg, it.samplePosition);
				else if (msg.isNoteOff())
					processNoteOff(msg);
				
				buffer.addEvent(msg, it.samplePosition);
			}
			midiMessages.swapWith(buffer);
		}

	private:
		MidiBuffer buffer;
		Voices voices;
		int channelIdx;

		void incChannel() noexcept
		{
			channelIdx = (channelIdx + 1) % NumChannelsMPE;
		}

		void processNoteOn(MidiMessage& msg, int ts)
		{
			for (auto ch = 0; ch < NumChannelsMPE; ++ch)
			{
				incChannel();
				auto& voice = voices[channelIdx];
				if (!voice.noteOn)
					return processNoteOn(voice, msg);
			}
			incChannel();
			auto& voice = voices[channelIdx];
			buffer.addEvent(MidiMessage::noteOff(voice.channel, voice.note), ts);
			processNoteOn(voice, msg);
		}

		void processNoteOn(Voice& voice, MidiMessage& msg) noexcept
		{
			const auto note = msg.getNoteNumber();
			voice.note = note;
			voice.channel = channelIdx + 2;
			voice.noteOn = true;
			msg.setChannel(voice.channel);
		}

		void processNoteOff(MidiMessage& msg) noexcept
		{
			for (auto ch = 0; ch < NumChannelsMPE; ++ch)
			{
				auto i = channelIdx - ch;
				if (i < 0)
					i += NumChannelsMPE;

				auto& voice = voices[i];
				if (voice.noteOn && voice.note == msg.getNoteNumber())
					return processNoteOff(voice, msg);
			}
		}

		void processNoteOff(Voice& voice, MidiMessage& msg) noexcept
		{
			msg.setChannel(voice.channel);
			voice.note = 0;
			voice.noteOn = false;
		}
	};
}