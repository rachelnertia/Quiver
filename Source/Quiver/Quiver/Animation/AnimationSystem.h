#pragma once

#include <chrono>
#include <unordered_map>
#include <vector>

#include "json.hpp"

#include "Quiver/Animation/AnimationId.h"
#include "Quiver/Animation/AnimationLibrary.h"
#include "Quiver/Animation/AnimatorId.h"
#include "Quiver/Animation/Rect.h"

namespace qvr {

struct AnimatorTarget {
	Animation::Rect rect;

	AnimatorTarget() = default;

	AnimatorTarget(const AnimatorTarget&) = delete;
	AnimatorTarget(const AnimatorTarget&&) = delete;

	AnimatorTarget& operator=(AnimatorTarget&) = delete;
	AnimatorTarget& operator=(AnimatorTarget&&) = delete;
};

class AnimatorRepeatSetting
{
public:
	explicit AnimatorRepeatSetting(const int repeatCount) : m_RepeatCount(repeatCount) {}

	AnimatorRepeatSetting() = default;

	int GetRepeatCount() const { return m_RepeatCount; }

	static const AnimatorRepeatSetting Forever;
	static const AnimatorRepeatSetting Never;
	static const AnimatorRepeatSetting Once;
	static const AnimatorRepeatSetting Twice;

private:
	int m_RepeatCount = Forever.GetRepeatCount();
};

struct AnimatorStartSetting
{
	AnimatorStartSetting(AnimationId animationId)
		: m_AnimationId(animationId)
	{}

	AnimatorStartSetting(
		AnimationId animationId,
		AnimatorRepeatSetting repeatSetting)
		: m_AnimationId(animationId)
		, m_RepeatSetting(repeatSetting)
	{}

	AnimationId m_AnimationId;
	AnimatorRepeatSetting m_RepeatSetting;
};

inline bool operator==(const AnimatorRepeatSetting& a, const AnimatorRepeatSetting& b) {
	return a.GetRepeatCount() == b.GetRepeatCount();
}

inline bool operator!=(const AnimatorRepeatSetting& a, const AnimatorRepeatSetting& b) {
	return a.GetRepeatCount() != b.GetRepeatCount();
}

class AnimationData;

class AnimationSystem {
public:
	AnimationSystem() = default;

	AnimationSystem(const AnimationSystem&) = delete;
	AnimationSystem(const AnimationSystem&&) = delete;
	AnimationSystem& operator=(const AnimationSystem&) = delete;
	AnimationSystem& operator=(const AnimationSystem&&) = delete;

	bool FromJson(const nlohmann::json& j);
	auto ToJson() const -> nlohmann::json;

	auto GetAnimations() const -> const AnimationLibrary& {
		return animations;
	}

	AnimationId AddAnimation(const AnimationData& anim);
	AnimationId AddAnimation(
		const AnimationData& anim, 
		const AnimationSourceInfo& animSourceInfo);

	bool RemoveAnimation(const AnimationId id);

	int GetReferenceCount(const AnimationId animation) const { 
		return animationReferenceCounts.at(animation); 
	}

	AnimatorId AddAnimator(
		AnimatorTarget& target, 
		const AnimatorStartSetting& startSetting);

	bool RemoveAnimator(const AnimatorId id);

	bool AnimatorExists(const AnimatorId id) const {
		return animators.states.count(id) != 0;
	}

	int GetAnimatorCount() const { 
		return animators.states.size(); 
	}

	void AnimatorGui(const AnimatorId id);

	bool SetAnimatorAnimation(
		const AnimatorId animatorId,
		const AnimatorStartSetting& animation,
		const bool clearQueue = true);

	bool SetAnimatorTarget(
		const AnimatorId id,
		AnimatorTarget& newTarget);

	bool SetAnimatorFrame(
		const AnimatorId id, 
		const int index);

	bool QueueAnimation(
		const AnimatorId animatorId,
		const AnimatorStartSetting& pendingAnimation);

	bool ClearAnimationQueue(const AnimatorId id);

	unsigned GetAnimatorFrame(const AnimatorId animatorId) const;

	AnimationId GetAnimatorAnimation(const AnimatorId animatorId) const;

	using TimeUnit = std::chrono::duration<int, std::milli>;

	void Animate(const TimeUnit ms);

	// animatorId  - id for an Animator whose Animation has alternate-viewpoint frames in it.
	// objectAngle - between 0 and tau (2 * pi), the angle the object is facing.
	// viewAngle   - between 0 and tau (2 * pi), the angle the object is being viewed from.
	void UpdateAnimatorAltView(const AnimatorId animatorId, const float objectAngle, const float viewAngle);

private:
	struct AnimatorAltViewState {
		// TODO: There is too much here.

		AnimatorAltViewState(const unsigned numAltViews)
			: numAltViews(numAltViews)
			, currentAltView(0)
		{}

		AnimatorAltViewState(const AnimatorAltViewState& other)
			: numAltViews(other.NumAltViews())
			, currentAltView(other.currentAltView)
		{}

		AnimatorAltViewState() = default;

		AnimatorAltViewState& operator=(const AnimatorAltViewState& other)
		{
			numAltViews = other.NumAltViews();
			currentAltView = other.currentAltView;
			return *this;
		}

		unsigned currentAltView = 0; // 0 = base frame

		unsigned NumAltViews() const { return numAltViews; }

	private:
		unsigned numAltViews = 0;
	};

	struct AnimatorState {
		AnimatorState(
			const AnimationId animation,
			const int frameIndex,
			const int index,
			AnimatorTarget& target,
			const AnimatorRepeatSetting& repeat,
			const AnimatorAltViewState& altViewState)
			: index(index)
			, currentFrame(frameIndex)
			, currentAnimation(animation)
			, target(&target)
			, repeatSetting(repeat)
			, repeatCount(0)
			, altViewState(altViewState)
		{}

		AnimatorState() = default;

		int index;
		int currentFrame;
		int repeatCount;
		AnimationId currentAnimation;
		AnimatorRepeatSetting repeatSetting;
		AnimatorAltViewState altViewState;
		AnimatorTarget* target;
		std::vector<AnimatorStartSetting> queuedAnimations;
	};

	struct AnimatorState_Hot {
		AnimatorState_Hot(
			const AnimatorId animatorId,
			const TimeUnit timeLeft)
			: animatorId(animatorId)
			, timeLeftInFrame(timeLeft)
		{}

		TimeUnit timeLeftInFrame;
		AnimatorId animatorId;
	};

	struct Animators {
		std::vector<AnimatorState_Hot> hotStates;
		std::unordered_map<AnimatorId, AnimatorState> states;
		AnimatorId GetNextAnimatorId();
	private:
		AnimatorId lastId = AnimatorId::Invalid;
	} animators;

	AnimationLibrary animations;

	std::unordered_map<AnimationId, unsigned> animationReferenceCounts;
};

// TODO: Put this in a different file.
struct AnimationSystemEditorData {
	char mFilenameBuffer[128] = { 0 };
	int mCurrentSelection = -1;
};

void GuiControls(AnimationSystem& animationSystem, AnimationSystemEditorData& editorData);

}