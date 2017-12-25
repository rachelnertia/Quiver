#pragma once

#include <chrono>
#include <unordered_map>
#include <vector>

#include <gsl/span>
#include <json.hpp>

#include "Quiver/Animation/AnimationId.h"
#include "Quiver/Animation/AnimationLibrary.h"
#include "Quiver/Animation/AnimatorId.h"
#include "Quiver/Animation/Rect.h"
#include "Quiver/Graphics/ViewBuffer.h"

namespace qvr {

struct AnimatorTarget {
	ViewBuffer views;

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

struct AnimationLibraryEditorData;

class AnimationData;

class AnimatorCollection {
public:
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

	AnimatorId Add(
		AnimatorTarget& target, 
		const AnimatorStartSetting& startSetting);

	bool Remove(const AnimatorId id);

	bool Exists(const AnimatorId id) const {
		return animators.states.count(id) != 0;
	}

	int GetCount() const { 
		return animators.states.size(); 
	}

	void AnimatorGui(const AnimatorId id);

	bool SetAnimation(
		const AnimatorId animatorId,
		const AnimatorStartSetting& animation,
		const bool clearQueue = true);

	bool SetTarget(
		const AnimatorId id,
		AnimatorTarget& newTarget);

	bool SetFrame(
		const AnimatorId id, 
		const int index);

	bool QueueAnimation(
		const AnimatorId animatorId,
		const AnimatorStartSetting& pendingAnimation);

	bool ClearAnimationQueue(const AnimatorId id);

	unsigned GetFrame(const AnimatorId animatorId) const;

	AnimationId GetAnimation(const AnimatorId animatorId) const;

	void Animate(const Animation::TimeUnit ms);

private:
	struct AnimatorState {
		AnimatorState(
			const AnimationId animation,
			const int frameIndex,
			const int index,
			AnimatorTarget& target,
			const AnimatorRepeatSetting& repeat)
			: index(index)
			, currentFrame(frameIndex)
			, currentAnimation(animation)
			, target(&target)
			, repeatSetting(repeat)
			, repeatCount(0)
		{}

		AnimatorState() = default;

		int index;
		int currentFrame;
		int repeatCount;
		AnimationId currentAnimation;
		AnimatorRepeatSetting repeatSetting;
		AnimatorTarget* target;
		std::vector<AnimatorStartSetting> queuedAnimations;
	};

	struct AnimatorState_Hot {
		AnimatorState_Hot(
			const AnimatorId animatorId,
			const Animation::TimeUnit timeLeft)
			: animatorId(animatorId)
			, timeLeftInFrame(timeLeft)
		{}

		Animation::TimeUnit timeLeftInFrame;
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

	// friends:

	friend void GuiControls(
		AnimatorCollection& animators, 
		AnimationLibraryEditorData& editorData);

	friend void to_json(nlohmann::json& j, const AnimatorCollection& animators);
	friend void from_json(const nlohmann::json& j, AnimatorCollection& animators);
};

void GuiControls(AnimatorCollection& animators, AnimationLibraryEditorData& editorData);

void to_json(nlohmann::json& j, const AnimatorCollection& animators);
void from_json(const nlohmann::json& j, AnimatorCollection& animators);

}