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

		using Voices = std::array<Voice, mpe::NumChannels>;

		AutoMPE() :
			buffer(),
			voices(),
			idx(0)
		{
			buffer.ensureSize(1024);
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
		int idx;

		void processNoteOn(MidiMessage& msg, int ts)
		{
			for (auto ch = 0; ch < NumChannels; ++ch)
			{
				idx = (idx + 1) & MaxChannel;
				auto& voice = voices[idx];
				if (!voice.noteOn)
				{
					const auto note = msg.getNoteNumber();
					voice.note = note;
					voice.channel = idx + 1;
					voice.noteOn = true;
					msg.setChannel(voice.channel);
					return;
				}	
			}
			idx = (idx + 1) & MaxChannel;
			auto& voice = voices[idx];
			buffer.addEvent(MidiMessage::noteOff(voice.channel, voice.note), ts);
			const auto note = msg.getNoteNumber();
			voice.note = note;
			voice.channel = idx + 1;
			voice.noteOn = true;
			msg.setChannel(voice.channel);
		}

		void processNoteOff(MidiMessage& msg) noexcept
		{
			for (auto ch = 0; ch < NumChannels; ++ch)
			{
				auto i = idx - ch;
				if (i < 0)
					i += NumChannels;

				auto& voice = voices[i];

				if (voice.noteOn && voice.note == msg.getNoteNumber())
				{
					msg.setChannel(voice.channel);
					voice.note = 0;
					voice.noteOn = false;
					return;
				}
			}
		}
	};
}