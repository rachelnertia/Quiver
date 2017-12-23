#include <catch.hpp>

#include "Quiver/Graphics/ViewBuffer.h"

using namespace qvr;
using Rect = Animation::Rect;

void GenerateRectValues(const gsl::span<Rect> rects) {
	int i = 0;
	for (auto& rect : rects) {
		rect = Rect{ i, i, i, i };
		i++;
	}
}

template<int size>
std::array<Rect, size> GenerateRects() {
	std::array<Rect, size> rects;
	GenerateRectValues(rects);
	return rects;
}

TEST_CASE("ViewBuffer", "[Graphics]") {
	ViewBuffer vb;

	REQUIRE(vb.viewCount == 0);

	SECTION("SetViews clamps input") {
		const auto rects = GenerateRects<vb.views.size() + 1>();

		SetViews(vb, rects);

		REQUIRE(vb.viewCount == (int)vb.views.size());
		REQUIRE(GetViews(vb) == gsl::make_span(&rects[0], vb.viewCount));
		REQUIRE(GetViews(vb) != gsl::make_span(rects));
	}

	SetViews(vb, GenerateRects<4>());  

	// Rotate the object and assert that we see the right frame from each angle.
	{
		const float Tau = 6.28318530718f;
		
		const float increment = Tau / vb.viewCount;
		const float offset = increment / 2;

		const float viewAngle = 0.0f;

		for (int i = 0; i < vb.viewCount; ++i) {
			const float objectAngle = (increment * i) + offset;

			const Rect r = CalculateView(vb, objectAngle, viewAngle);
			
			REQUIRE(r == vb.views[i]);
		}
	}
}
