#pragma once
#include <array>
#include <functional>
#include "Axiom.h"
#include "Math.h"
#include "mts/Master/libMTSMaster.h"
#include <juce_core/juce_core.h>

namespace mts
{
	class Xen
	{
		using String = juce::String;
		static constexpr int NumPitches = 128;
	public:
		Xen();

		void reset() noexcept;

		// xen, anchorPitch, anchorFreq, useNearest
		void operator()(double, double, double, bool) noexcept;
	private:
		double freqTable[NumPitches];
		double xen, anchorPitch, anchorFreq;
		String name;
		bool useNearest;

		void update() noexcept;
	};
}