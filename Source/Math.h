#pragma once
#include <cmath>
#include <limits>

namespace math
{
	double noteToFreq(double note, double xen = 12., double basePitch = 69., double masterTune = 440.) noexcept
	{
		return masterTune * std::exp2((note - basePitch) / xen);
	}

	double freqToNote(double freq, double xen = 12., double basePitch = 69., double masterTune = 440.) noexcept
	{
		return basePitch + xen * std::log2(freq / masterTune);
	}

	double closestFreq(double freq, double xen = 12., double basePitch = 69., double masterTune = 440.) noexcept
	{
		auto closestFreq = 0.;
		auto closestDist = std::numeric_limits<double>::max();

		for (auto note = 0; note < 128; ++note)
		{
			const auto nFreq = noteToFreq(static_cast<double>(note), xen, basePitch, masterTune);
			const auto nDist = std::abs(freq - nFreq);
			if (nDist < closestDist)
			{
				closestDist = nDist;
				closestFreq = nFreq;
			}
		}

		return closestFreq;
	}
}