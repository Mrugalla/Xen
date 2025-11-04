#pragma once
#include <functional>
#include "MPESplit.h"
#include "Math.h"
#include "mts/Client/libMTSClient.h"

namespace syn
{
	using MidiBuffer = juce::MidiBuffer;
	using MidiMessage = juce::MidiMessage;

	class Voice
	{
		struct Osc
		{
			static constexpr double Gain = .2;

			Osc() :
				phase(0.),
				inc(0.),
				env(0.),
				rise(0.),
				fall(0.),
				sampleRateInvTau(1.),
				noteOn(false)
			{}

			void prepare(double sampleRate) noexcept
			{
				sampleRateInvTau = math::Tau / sampleRate;
				rise = 500. / sampleRate;
				fall = 80. / sampleRate;
			}

			void setFreqHz(double freq) noexcept
			{
				inc = freq * sampleRateInvTau;
			}

			double operator()() noexcept
			{
				phase += inc;
				if (phase > math::Pi)
					phase -= math::Tau;
				const auto sine = std::sin(phase);
				const auto saturated = std::tanh(4. * sine) * Gain;

				if (noteOn)
					env += rise * (1. - env);
				else
					env += fall * (0. - env);

				const auto y = saturated * env;
				return y;
			}

		private:
			double phase, inc, env, rise, fall, sampleRateInvTau;
		public:
			bool noteOn;
		};
	public:
		using PitchesArray = std::array<double, 128>;

		Voice() :
			osc(),
			curNote(0)
		{
		}

		void prepare(double sampleRate)
		{
			osc.prepare(sampleRate);
			curNote = 0;
		}

		void update(const PitchesArray& pitches) noexcept
		{
			osc.setFreqHz(pitches[curNote]);
		}

		void synthMPE(float* smpls, const MidiBuffer& midiIn,
			const PitchesArray& pitches, int numSamples) noexcept
		{
			auto s = 0;
			for (const auto it : midiIn)
			{
				const auto ts = it.samplePosition;
				const auto msg = it.getMessage();
				if (msg.isNoteOnOrOff())
				{
					synthesize(smpls, s, ts);
					s = ts;
					if (msg.isNoteOn())
					{
						curNote = msg.getNoteNumber();
						const auto freq = pitches[curNote];
						osc.setFreqHz(freq);
						osc.noteOn = true;
					}
					else if(msg.isNoteOff())
						osc.noteOn = false;
				}
			}
			synthesize(smpls, s, numSamples);
		}

		void synthMTS(float* smpls, const MidiBuffer& midiIn,
			MTSClient* client, int numSamples) noexcept
		{
			auto s = 0;
			for (const auto it : midiIn)
			{
				const auto ts = it.samplePosition;
				const auto msg = it.getMessage();
				if (msg.isNoteOnOrOff())
				{
					synthesize(smpls, s, ts);
					s = ts;
					if (msg.isNoteOn())
					{
						curNote = msg.getNoteNumber();
						auto freq = MTS_NoteToFrequency(client, static_cast<char>(curNote), static_cast<char>(-1));
						osc.setFreqHz(freq);
						osc.noteOn = true;
					}
					else if (msg.isNoteOff())
						osc.noteOn = false;
				}
			}
			synthesize(smpls, s, numSamples);
		}
	private:
		Osc osc;
		int curNote;

		void synthesize(float* smpls, int s, int ts) noexcept
		{
			while (s < ts)
			{
				smpls[s] += static_cast<float>(osc());
				++s;
			}
		}
	};

	struct Synth
	{
		using MPE = mpe::MPESplit;

		Synth(MPE& _mpe) :
			pitches(),
			mtsClient(MTS_RegisterClient()),
			voices(),
			mpe(_mpe),
			xen(0.),
			anchorPitch(69.),
			anchorFreq(440.),
			useNearest(false)
		{}

		~Synth()
		{
			MTS_DeregisterClient(mtsClient);
		}

		void updateParameters(double _xen, double _anchorPitch, double _anchorFreq, bool _useNearest) noexcept
		{
			if(xen == _xen &&
			   anchorPitch == _anchorPitch &&
			   anchorFreq == _anchorFreq &&
			   useNearest == _useNearest)
				return;
			xen = _xen;
			anchorPitch = _anchorPitch;
			anchorFreq = _anchorFreq;
			useNearest = _useNearest;
			const auto freqFunc = useNearest ?
				[](double pitch, double xen, double anchorPitch, double anchorFreq)
				{
					return math::noteToFreqIn12Steps(pitch, xen, anchorPitch, anchorFreq);
				} :
				[](double pitch, double xen, double anchorPitch, double anchorFreq)
					{
						return math::noteToFreq(pitch, xen, anchorPitch, anchorFreq);
					};
				for (int i = 0; i < 128; ++i)
					pitches[i] = freqFunc(static_cast<double>(i), xen, anchorPitch, anchorFreq);
			for(auto& voice: voices)
				voice.update(pitches);
		}

		void prepare(double sampleRate) noexcept
		{
			for (auto& voice : voices)
				voice.prepare(sampleRate);
			xen = 0.;
		}

		void synthMPE(float* const* samples, int numSamples) noexcept
		{
			clear(samples, numSamples);
			for (auto ch = 0; ch < mpe::NumChannelsMPE; ++ch)
			{
				auto& voice = voices[ch];
				const auto& midi = mpe[ch + 2];
				voice.synthMPE
				(
					samples[0], midi, pitches, numSamples
				);
			}
			copy(samples, numSamples);
		}

		void synthMTS(float* const* samples, int numSamples) noexcept
		{
			clear(samples, numSamples);
			for (auto ch = 0; ch < mpe::NumChannelsMPE; ++ch)
			{
				auto& voice = voices[ch];
				const auto& midi = mpe[ch + 2];
				voice.synthMTS
				(
					samples[0], midi, mtsClient, numSamples
				);
			}
			copy(samples, numSamples);
		}
	private:
		Voice::PitchesArray pitches;
		MTSClient* mtsClient;
		std::array<Voice, mpe::NumChannelsMPE> voices;
		MPE& mpe;
		double xen, anchorPitch, anchorFreq;
		bool useNearest;

		void clear(float* const* samples, int numSamples) noexcept
		{
			for(auto ch = 0; ch < 2; ++ch)
				juce::FloatVectorOperations::clear(samples[ch], numSamples);
		}

		void copy(float* const* samples, int numSamples) noexcept
		{
			juce::FloatVectorOperations::copy(samples[1], samples[0], numSamples);
		}
	};
}