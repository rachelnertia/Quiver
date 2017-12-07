#pragma once

#include "Quiver/Animation/AnimationId.h"

namespace qvr
{

class AnimationLibrary;

AnimationId PickAnimation(const AnimationLibrary& animations, int& currentSelection);

}