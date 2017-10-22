#pragma once

#include <SFML/Graphics/Texture.hpp>

#include "AnimationSystem.h"
#include "AnimationData.h"

namespace qvr {

class AnimationEditor {
public:
	void ProcessGui();

	void Update(const float dt);

	bool IsOpen() const { return mIsOpen; }

	void SetOpen(const bool open) { mIsOpen = open; }

private:
	using AnimationCollection = std::vector<std::pair<std::string, AnimationData>>;

	AnimationCollection mAnimationCollection;

	AnimationData mCurrentAnim;

	std::string mCurrentAnimName;

	bool mIsOpen = false;
	bool mTiled = false;

	Animation::Rect::Dimensions mTileDimensions = { 1, 1 };
	int mCurrentFrameIndex = 0;
	int mCurrentViewIndex = 0;

	char mTextureFilenameBuffer[128] = { 0 };
	char mAnimationToLoadFilename[128] = { 0 };
	char mAnimationCollectionFilename[128] = { 0 };

	sf::Texture mTexture;

	AnimationSystem mAnimationSystem;
	AnimationId mCurrentAnimationId = AnimationId::Invalid;
	AnimatorId  mAnimatorId = AnimatorId::Invalid;
	AnimatorTarget mAnimationPreviewRect;

	float mAnimationPlaybackSpeedMultiplier = 1.0f;
	float mTimeCounter = 0.0f;
	float mAnimationPreviewViewAngle = 0.0f;
	float mAnimationPreviewObjectAngle = 0.0f;
};

}