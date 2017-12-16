#pragma once

#include "Quiver/Animation/AnimationId.h"

namespace qvr
{

class AnimationLibrary;

struct AnimationLibraryEditorData {
	char mFilenameBuffer[128] = { 0 };
	int mCurrentSelection = -1;
};

AnimationId PickAnimation(const AnimationLibrary& animations, int& currentSelection);

void AddAnimations(AnimationLibrary& animations, AnimationLibraryEditorData& editorData);

}