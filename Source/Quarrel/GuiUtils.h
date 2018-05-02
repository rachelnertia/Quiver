#pragma once

#include <optional.hpp>

#include <Quiver/Animation/AnimationId.h>

namespace qvr {
class AnimatorCollection;
}

std::experimental::optional<qvr::AnimationId> PickAnimationGui(
	const char* title,
	const qvr::AnimationId currentAnim,
	const qvr::AnimatorCollection& animators);

