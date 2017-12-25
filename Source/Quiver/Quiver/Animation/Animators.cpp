#include "Animators.h"

#include <fstream>
#include <iostream>
#include <set>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

#include <ImGui/imgui.h>

#include "Quiver/Animation/AnimationData.h"
#include "Quiver/Animation/AnimationLibraryGui.h"
#include "Quiver/Misc/ImGuiHelpers.h"
#include "Quiver/Misc/JsonHelpers.h"
#include "Quiver/Misc/Logging.h"

namespace qvr {

using namespace Animation;

const AnimatorRepeatSetting AnimatorRepeatSetting::Forever = AnimatorRepeatSetting(-1);
const AnimatorRepeatSetting AnimatorRepeatSetting::Never = AnimatorRepeatSetting(0);
const AnimatorRepeatSetting AnimatorRepeatSetting::Once = AnimatorRepeatSetting(1);
const AnimatorRepeatSetting AnimatorRepeatSetting::Twice = AnimatorRepeatSetting(2);

AnimationId AnimatorCollection::AddAnimation(const AnimationData & anim) 
{
	return animations.Add(anim);
}

AnimationId AnimatorCollection::AddAnimation(const AnimationData& data, const AnimationSourceInfo& sourceInfo)
{
	return animations.Add(data, sourceInfo);
}

bool AnimatorCollection::RemoveAnimation(const AnimationId id)
{
	auto log = spdlog::get("console");
	assert(log);

	std::string logCtx = fmt::format("AnimatorCollection::RemoveAnimation({}): ", id);

	// Animators that reference this Animation are now invalid, so we need to delete them.
	{
		std::vector<AnimatorId> animatorsToRemove;

		for (const auto& animator : animators.states) {
			if (animator.second.currentAnimation == id) {
				animatorsToRemove.push_back(animator.first);
			}
		}

		log->debug(
			"{} Found {} Animators that reference this Animation. Removing them...", 
			logCtx, 
			animatorsToRemove.size());

		for (auto animatorId : animatorsToRemove) {
			Remove(animatorId);
		}
	}

	return animations.Remove(id);
}

AnimatorId AnimatorCollection::Animators::GetNextAnimatorId() {
	lastId = AnimatorId(lastId.GetValue() + 1);
	// TODO: Check for lastId == Invalid and increment if so.
	return lastId;
}

AnimatorId AnimatorCollection::Add(
	AnimatorTarget & target,
	const AnimatorStartSetting& startSetting)
{
	assert(startSetting.m_AnimationId != AnimationId::Invalid);
	if (startSetting.m_AnimationId == AnimationId::Invalid) return AnimatorId::Invalid;
	if (!animations.Contains(startSetting.m_AnimationId))   return AnimatorId::Invalid;

	const int frameIndex = 0;

	const AnimatorId newAnimatorId = animators.GetNextAnimatorId();

	animators.states[newAnimatorId] =
		AnimatorState(
			startSetting.m_AnimationId,
			frameIndex,
			animators.hotStates.size(),
			target,
			startSetting.m_RepeatSetting);

	animators.hotStates.emplace_back(
		newAnimatorId,
		animations.GetTime(startSetting.m_AnimationId, 0));

	// Update target.
	SetViews(
		target.views,
		animations.GetRects(startSetting.m_AnimationId, 0));

	animationReferenceCounts[startSetting.m_AnimationId]++;

	return newAnimatorId;
}

bool AnimatorCollection::Remove(const AnimatorId id)
{
	assert(id != AnimatorId::Invalid);

	// TODO: Add logging.

	if (animators.states.count(id) == 0) return false;

	{
		const AnimatorState& animator = animators.states[id];

		animationReferenceCounts[animator.currentAnimation]--;

		// swap...
		std::iter_swap(
			animators.hotStates.begin() + animator.index,
			animators.hotStates.end() - 1);

		// Update the AnimatorState whose hot state was swapped, so that
		// it knows the new location of its hot state in the array.
		const AnimatorId idOfMovedAnimator =
			animators.hotStates[animator.index].animatorId;
		animators.states.at(idOfMovedAnimator).index = animator.index;

		// ...and pop!
		animators.hotStates.pop_back();
	}

	animators.states.erase(id);

	return true;
}

void AnimatorCollection::AnimatorGui(const AnimatorId id)
{
	if (!Exists(id)) {
		ImGui::Text("Animator #%u does not exist", id);
		return;
	}

	const AnimatorState& animator = animators.states[id];

	ImGui::Text("Animation:  \t#%u", animator.currentAnimation);
	ImGui::Text("Target Rect:\t0x%08X", animator.target);

	{
		const int numFrames = (int)animations.GetFrameCount(animator.currentAnimation);
		int frameIndex = (int)animator.currentFrame;
		if (ImGui::SliderInt("Current Frame", &frameIndex, 0, numFrames - 1))
			SetFrame(id, frameIndex);
	}
}

bool AnimatorCollection::ClearAnimationQueue(const AnimatorId id)
{
	if (!Exists(id)) return false;

	AnimatorState& animator = animators.states[id];

	animator.queuedAnimations.clear();

	return true;
}

bool AnimatorCollection::SetAnimation(const AnimatorId animatorId, const AnimatorStartSetting& startSetting, const bool clearQueue)
{
	if (!Exists(animatorId)) return false;
	if (!animations.Contains(startSetting.m_AnimationId)) return false;

	AnimatorState& animator = animators.states[animatorId];

	// Decrease refcount on current animation.
	animationReferenceCounts[animator.currentAnimation]--;
	// Increase refcount on new animation.
	animationReferenceCounts[startSetting.m_AnimationId]++;

	animator.currentAnimation = startSetting.m_AnimationId;
	animator.currentFrame = 0;
	animator.repeatCount = 0;
	animator.repeatSetting = startSetting.m_RepeatSetting;

	if (clearQueue) {
		animator.queuedAnimations.clear();
	}

	SetViews(
		animator.target->views,
		animations.GetRects(animator.currentAnimation, animator.currentFrame));

	animators.hotStates[animator.index].timeLeftInFrame
		= animations.GetTime(
			animator.currentAnimation,
			animator.currentFrame);

	return true;
}

bool AnimatorCollection::SetTarget(const AnimatorId id, AnimatorTarget & newTarget)
{
	if (!Exists(id)) return false;

	AnimatorState& animator = animators.states[id];

	animator.target = &newTarget;

	return true;
}

bool AnimatorCollection::QueueAnimation(const AnimatorId id, const AnimatorStartSetting& pendingAnimation)
{
	auto log = spdlog::get("console");
	assert(log);

	const auto logCtx = "AnimatorCollection::QueueAnimation:";

	if (!Exists(id)) {
		log->error("{} Animator {} does not exist.", logCtx, id);
		return false;
	}

	animators.states[id].queuedAnimations.push_back(
		pendingAnimation);

	return true;
}

AnimationId AnimatorCollection::GetAnimation(const AnimatorId animatorId) const
{
	assert(Exists(animatorId));
	return animators.states.at(animatorId).currentAnimation;
}

unsigned AnimatorCollection::GetFrame(const AnimatorId animatorId) const
{
	assert(Exists(animatorId));
	return animators.states.at(animatorId).currentFrame;
}

bool AnimatorCollection::SetFrame(
	const AnimatorId id,
	const int frameIndex)
{
	if (frameIndex < 0) return false;

	if (!Exists(id)) return false;

	AnimatorState& animator = animators.states[id];

	if (frameIndex >= animations.GetFrameCount(animator.currentAnimation))
		return false;

	animator.currentFrame = frameIndex;

	SetViews(
		animator.target->views,
		animations.GetRects(animator.currentAnimation, animator.currentFrame));

	animators.hotStates[animator.index].timeLeftInFrame =
		animations.GetTime(
			animator.currentAnimation, 
			animator.currentFrame);

	return true;
}

using namespace std::chrono_literals;

void AnimatorCollection::Animate(const TimeUnit time) {
	// because this system doesn't support rewinding.
	assert(time >= TimeUnit::zero());

	for (auto& animatorHotState : animators.hotStates) {
		animatorHotState.timeLeftInFrame -= time;
	}

	// select entries for which timeLeftInFrame <= 0:
	std::vector<AnimatorId> animatorsToUpdate;

	for (auto& animatorHotState : animators.hotStates) {
		if (animatorHotState.timeLeftInFrame <= TimeUnit::zero()) {
			animatorsToUpdate.push_back(animatorHotState.animatorId);
		}
	}

	std::vector<AnimatorId> animatorsToRemove;

	for (auto animatorId : animatorsToUpdate) {
		AnimatorState& animator = animators.states.at(animatorId);

		animator.currentFrame = 
			(animator.currentFrame + 1) % animations.GetFrameCount(animator.currentAnimation);

		// This Animator is returning to the beginning of the Animation.
		if (animator.currentFrame == 0)
		{
			const AnimatorRepeatSetting repeatSetting =
				animator.repeatSetting;

			if (repeatSetting != AnimatorRepeatSetting::Forever)
			{
				if (animator.repeatCount >= repeatSetting.GetRepeatCount())
				{
					if (animator.queuedAnimations.empty())
					{
						animatorsToRemove.push_back(animatorId);
					}
					else
					{
						const bool clearQueue = false;

						if (!SetAnimation(
							animatorId,
							animator.queuedAnimations.front(),
							clearQueue))
						{
							animatorsToRemove.push_back(animatorId);
						}

						// Pop the front of the queue.
						animator.queuedAnimations.erase(
							animator.queuedAnimations.begin());
					}

					continue;
				}

				animator.repeatCount++;
			}
		}

		// Update target.
		SetViews(
			animator.target->views,
			animations.GetRects(animator.currentAnimation, animator.currentFrame));

		animators.hotStates[animator.index].timeLeftInFrame += 
			animations.GetTime(animator.currentAnimation, animator.currentFrame);
	}

	for (auto animatorId : animatorsToRemove) {
		Remove(animatorId);
	}
}

void GuiControls(AnimatorCollection& animators, AnimationLibraryEditorData& editorData)
{
	ImGui::Text("Num. Animations: %u", animators.GetAnimations().GetCount());
	ImGui::Text("Num. Animators:  %u", animators.GetCount());

	if (ImGui::CollapsingHeader("View Animations"))
	{
		ImGui::AutoIndent indent;

		const AnimationId anim = 
			PickAnimation(animators.GetAnimations(), editorData.mCurrentSelection);

		if (anim != AnimationId::Invalid)
		{
			AnimationSourceInfo animSourceInfo =
				animators.GetAnimations().GetSourceInfo(anim).value();

			ImGui::Text("Name: %s", animSourceInfo.name.c_str());
			ImGui::Text("File: %s", animSourceInfo.filename.c_str());
			ImGui::Text("Ref Count: %d", animators.GetReferenceCount(anim));

			if (ImGui::Button("Remove")) {
				animators.RemoveAnimation(anim);
			}
		}
	}

	if (ImGui::CollapsingHeader("Add Animations"))
	{
		ImGui::AutoIndent indent;

		AddAnimations(animators.animations, editorData);
	}
}

using json = nlohmann::json;

void to_json(json& j, const AnimatorCollection& animators) {
	j = animators.animations;
}

void from_json(const json& j, AnimatorCollection& animators) {
	animators.animations = j;
}

}