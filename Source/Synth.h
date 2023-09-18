#pragma once
#include "MPESplit.h"
#include "Math.h"

namespace syn
{
	using MidiBuffer = juce::MidiBuffer;
	static constexpr double Pi = 3.1415926535897932384626433832795;
	static constexpr double Tau = 2. * Pi;

	class XenSynth
	{
		struct Synth
		{
			static constexpr double Gain = .2;

			Synth() :
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
				sampleRateInvTau = Tau / sampleRate;
				rise = 500. / sampleRate;
				fall = 80. / sampleRate;
			}

			void setFreqHz(double freq) noexcept
			{
				inc = freq * sampleRateInvTau;
			}

			double process() noexcept
			{
				phase += inc;
				if (phase > Pi)
					phase -= Tau;
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
		XenSynth() :
			synth()
		{
		}

		void prepare(double sampleRate)
		{
			synth.prepare(sampleRate);
		}

		void synthesizeRescale(float* smpls, const MidiBuffer& midiIn,
			double xen, double basePitch,
			double masterTune, int numSamples)
		{
			auto s = 0;

			for (const auto it : midiIn)
			{
				const auto ts = it.samplePosition;
				const auto msg = it.getMessage();
				if (msg.isNoteOnOrOff())
				{
					while (s < ts)
					{
						smpls[s] += static_cast<float>(synth.process());
						++s;
					}
					if (msg.isNoteOn())
					{
						const auto noteNumber = static_cast<double>(msg.getNoteNumber());
						const auto freq = math::noteToFreq(noteNumber, xen, basePitch, masterTune);
						synth.setFreqHz(freq);
						synth.noteOn = true;
					}
					else
						synth.noteOn = false;
				}
			}

			while (s < numSamples)
			{
				smpls[s] += static_cast<float>(synth.process());
				++s;
			}
		}

		void synthesizeNearest(float* smpls, const MidiBuffer& midiIn,
			double xen, double basePitch,
			double masterTune, int numSamples)
		{
			auto s = 0;

			for (const auto it : midiIn)
			{
				const auto ts = it.samplePosition;
				const auto msg = it.getMessage();
				if (msg.isNoteOnOrOff())
				{
					while (s < ts)
					{
						smpls[s] += static_cast<float>(synth.process());
						++s;
					}

					if (msg.isNoteOn())
					{
						const auto noteNumber = static_cast<double>(msg.getNoteNumber());
						const auto freq = math::noteToFreq(noteNumber);
						const auto cFreq = math::closestFreq(freq, xen, basePitch, masterTune);
						synth.setFreqHz(cFreq);
						synth.noteOn = true;
					}
					else
						synth.noteOn = false;
				}
			}

			while (s < numSamples)
			{
				smpls[s] += static_cast<float>(synth.process());
				++s;
			}
		}

	private:
		Synth synth;
	};

	struct XenSynthMPE
	{
		using MPE = mpe::MPESplit;
		static constexpr int NumChannels = 16;

		XenSynthMPE(MPE& _mpe) :
			voices(),
			mpe(_mpe)
		{}

		void prepare(double sampleRate)
		{
			for (auto& voice : voices)
				voice.prepare(sampleRate);
		}

		void operator()(float* const* samples,
			double xen, double basePitch, double masterTune,
			int numChannels, int numSamples,
			bool playModeVal) noexcept
		{
			if (playModeVal)
				for (auto ch = 1; ch <= mpe::NumChannels; ++ch)
				{
					const auto& midi = mpe[ch];
					auto& voice = voices[ch - 1];

					voice.synthesizeNearest(samples[0], midi, xen, basePitch, masterTune, numSamples);
				}
			else
				for (auto ch = 1; ch <= mpe::NumChannels; ++ch)
				{
					const auto& midi = mpe[ch];
					auto& voice = voices[ch - 1];

					voice.synthesizeRescale(samples[0], midi, xen, basePitch, masterTune, numSamples);
				}

			if (numChannels == 2)
				juce::FloatVectorOperations::copy(samples[1], samples[0], numSamples);
		}

	private:
		std::array<XenSynth, NumChannels> voices;
		MPE& mpe;
	};
}