#pragma once
#include <cmath>
#include <limits>

namespace math
{
	static constexpr double Pi = 3.1415926535897932384626433832795;
	static constexpr double Tau = 2. * Pi;

	inline double noteToFreq(double note, double xen = 12., double anchorPitch = 69., double anchorFreq = 440.) noexcept
	{
		return anchorFreq * std::exp2((note - anchorPitch) / xen);
	}

	inline double freqToNote(double freq, double xen = 12., double anchorPitch = 69., double anchorFreq = 440.) noexcept
	{
		return xen * std::log2(freq / anchorFreq) + anchorPitch;
	}

	inline double noteToFreqIn12Steps(double note, double xen = 12., double anchorPitch = 69., double anchorFreq = 440.) noexcept
	{
		const auto freq12 = noteToFreq(note, 12., 69., anchorFreq);
		const auto noteXen = freqToNote(freq12, xen, anchorPitch, anchorFreq);
		const auto closest = std::round(noteXen);
		return noteToFreq(closest, xen, anchorPitch, anchorFreq);
	}
}