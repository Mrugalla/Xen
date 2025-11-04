#pragma once
#include <juce_core/juce_core.h>
#include <array>
#include <functional>
#include "mts/Master/libMTSMaster.h"
#include "Axiom.h"
#include "Math.h"
#include "MPESplit.h"
#include "Synth.h"
#include "XenRescaler.h"

namespace xen
{
	using Timer = juce::Timer;

	struct Xen :
		public Timer
	{
		static constexpr int NumPitches = 128;
		using String = juce::String;
		using Midi = juce::MidiBuffer;

		Xen(mpe::Split& mpeSplit) :
			Timer(),
			freqTable(),
			xen(0.),
			anchorPitch(0.),
			anchorFreq(0.),
			pbRange(0.),
			stepsIn12(false),
			mtsEnabled(false),
			synth(mpeSplit),
			rescaler(mpeSplit),
			name("")
		{
			if (MTS_CanRegisterMaster())
				MTS_RegisterMaster();
			startTimerHz(1);
		}

		~Xen()
		{
			MTS_DeregisterMaster();
		}

		void timerCallback() override
		{
			if (MTS_CanRegisterMaster())
				MTS_RegisterMaster();
		}

		void updateParameters(double _xen, double _anchorPitch, double _anchorFreq,
			double _pbRange, bool _mtsEnabled, bool _stepsIn12) noexcept
		{
			if (_mtsEnabled)
			{
				if (!mtsEnabled)
				{
					mtsEnabled = true;
					forceUpdate();
				}
				if (xen != _xen ||
					anchorPitch != _anchorPitch ||
					anchorFreq != _anchorFreq ||
					stepsIn12 != _stepsIn12)
				{
					xen = _xen;
					anchorPitch = _anchorPitch;
					anchorFreq = _anchorFreq;
					stepsIn12 = _stepsIn12;
					updateFreqTable();
					updateMTS();
				}
			}
			else
			{
				if (mtsEnabled)
				{
					mtsEnabled = false;
					xen = 12.;
					anchorPitch = 69.;
					anchorFreq = 440.;
					stepsIn12 = true;
					updateFreqTable();
					updateMTS();
				}
				if (xen != _xen ||
					anchorPitch != _anchorPitch ||
					anchorFreq != _anchorFreq ||
					stepsIn12 != _stepsIn12)
				{
					xen = _xen;
					anchorPitch = _anchorPitch;
					anchorFreq = _anchorFreq;
					stepsIn12 = _stepsIn12;
					updateFreqTable();
				}
			}
			
			if(pbRange != _pbRange)
			{
				pbRange = _pbRange;
				rescaler.update(pbRange);
			}
		}

		void prepare(double sampleRate) noexcept
		{
			synth.prepare(sampleRate);
			forceUpdate();
		}

		void operator()(float* const* samples, Midi& midi, int numSamples)
		{
			if (mtsEnabled)
				return synth.synthMTS(samples, numSamples);
			synth.synthMPE(samples, freqTable, numSamples);
			rescaler(midi, freqTable, numSamples);
		}

	private:
		double freqTable[NumPitches];
		double xen, anchorPitch, anchorFreq, pbRange;
		bool stepsIn12, mtsEnabled;

		syn::Synth synth;
		XenRescalerMPE rescaler;
		String name;

		void forceUpdate() noexcept
		{
			xen = static_cast<double>(axiom::MinXen - 1);
		}

		void updateFreqTable() noexcept
		{
			const auto hzFunc = stepsIn12 ?
				[](double pitch, double xen, double anchorPitch, double anchorFreq)
				{
					return math::noteToFreqIn12Steps(pitch, xen, anchorPitch, anchorFreq);
				} :
				[](double pitch, double xen, double anchorPitch, double anchorFreq)
				{
					return math::noteToFreq(pitch, xen, anchorPitch, anchorFreq);
				};
				for (int i = 0; i < NumPitches; ++i)
				{
					const auto iD = static_cast<double>(i);
					const auto hz = hzFunc(iD, xen, anchorPitch, anchorFreq);
					freqTable[i] = hz;
				}
				synth.update(&freqTable[0]);
		}

		void updateMTS()
		{
			MTS_SetNoteTunings(freqTable);
			name = String(xen) + " edo";
			MTS_SetScaleName(name.getCharPointer());
		}
	};
}