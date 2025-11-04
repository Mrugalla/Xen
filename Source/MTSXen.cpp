#include "MTSXen.h"

namespace mts
{
	Xen::Xen() :
		freqTable(),
		xen(0.),
		anchorPitch(69.),
		anchorFreq(440.),
		name(),
		useNearest(false)
	{
	}

	void Xen::reset() noexcept
	{
		xen = 0.;
	}

	void Xen::operator()(double _xen,
		double _anchorPitch, double _anchorFreq, bool _useNearest) noexcept
	{
		if (xen == _xen &&
			anchorPitch == _anchorPitch &&
			anchorFreq == _anchorFreq &&
			useNearest == _useNearest)
			return;
		xen = _xen;
		anchorPitch = _anchorPitch;
		anchorFreq = _anchorFreq;
		useNearest = _useNearest;
		update();
	}

	void Xen::update() noexcept
	{
		const auto hzFunc = useNearest ?
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
			MTS_SetNoteTunings(freqTable);
			name = String(xen) + " edo";
			MTS_SetScaleName(name.getCharPointer());
	}
}