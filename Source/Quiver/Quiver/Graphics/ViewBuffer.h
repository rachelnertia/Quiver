#pragma once

#include <array>

#include <gsl/span>

#include "Quiver/Animation/Rect.h"

namespace qvr {

struct ViewBuffer {
	int viewCount = 0;
	std::array<Animation::Rect, 8> views;

	// TODO: Consider making initial value of viewCount == 1.
	// TODO: Consider making viewCount < 1 illegal.
};

// TODO: Consider making these public member functions of ViewBuffer and
// making the data members private.

inline void SetView(
	ViewBuffer& vb,
	const Animation::Rect& singleView)
{
	vb.viewCount = 1;
	vb.views[0] = singleView;
}

inline void SetViews(
	ViewBuffer& target,
	const gsl::span<const Animation::Rect> newViews)
{
	target.viewCount =
		std::min(
		(int)newViews.length(),
			(int)target.views.max_size());

	std::copy(
		std::begin(newViews),
		std::begin(newViews) + target.viewCount,
		std::begin(target.views));
}

inline auto GetViews(const ViewBuffer& vb) -> gsl::span<const Animation::Rect> {
	return gsl::make_span(&vb.views[0], vb.viewCount);
}

inline const Animation::Rect& CalculateView(
	const ViewBuffer& vb, 
	const float objectAngle, 
	const float viewAngle)
{
	assert(vb.viewCount > 0);

	const float Tau = 6.28318530718f;

	const int viewIndex =
		(int)((fmodf((objectAngle - viewAngle + Tau), Tau) / Tau) * (vb.viewCount));

	// Make sure my maths is right.
	assert(viewIndex >= 0);
	assert(viewIndex < vb.viewCount);

	return vb.views[viewIndex];
}

}