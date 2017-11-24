#include "AnimationSystem.h"

#include <fstream>
#include <iostream>
#include <set>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

#include <ImGui/imgui.h>

#include "Quiver/Animation/AnimationData.h"
#include "Quiver/Misc/ImGuiHelpers.h"
#include "Quiver/Misc/JsonHelpers.h"

namespace qvr {

using namespace Animation;

const AnimatorRepeatSetting AnimatorRepeatSetting::Forever = AnimatorRepeatSetting(-1);
const AnimatorRepeatSetting AnimatorRepeatSetting::Never = AnimatorRepeatSetting(0);
const AnimatorRepeatSetting AnimatorRepeatSetting::Once = AnimatorRepeatSetting(1);
const AnimatorRepeatSetting AnimatorRepeatSetting::Twice = AnimatorRepeatSetting(2);

bool AnimationSystem::FromJson(const nlohmann::json & j)
{
	animations = j;

	return true;
}

// Store the source info of every Animation currently in the System so that we can reload later.
nlohmann::json AnimationSystem::ToJson() const
{
	return qvr::ToJson(animations);
}

AnimationId AnimationSystem::AddAnimation(const AnimationData & anim) 
{
	return animations.Add(anim);
}

AnimationId AnimationSystem::AddAnimation(const AnimationData& data, const AnimationSourceInfo& sourceInfo)
{
	return animations.Add(data, sourceInfo);
}

bool AnimationSystem::RemoveAnimation(const AnimationId id)
{
	auto log = spdlog::get("console");
	assert(log);

	std::string logCtx = fmt::format("AnimationSystem::RemoveAnimation({}): ", id);

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
			RemoveAnimator(animatorId);
		}
	}

	return animations.Remove(id);
}

bool AnimationSystem::GetAnimationSourceInfo(const AnimationId id, AnimationSourceInfo & sourceInfoOut) const
{
	std::experimental::optional<AnimationSourceInfo> sourceInfo = animations.GetSourceInfo(id);

	if (sourceInfo) {
		sourceInfoOut = sourceInfo.value();
		return true;
	}

	return false;
}

unsigned AnimationSystem::GetAnimationNumFrames(const AnimationId id) const
{
	return animations.GetFrameCount(id);
}

bool AnimationSystem::AnimationHasAltViews(const AnimationId id) const
{
	return animations.HasAltViews(id);
}

AnimationId AnimationSystem::GetAnimationFromSource(const AnimationSourceInfo & animSource) const
{
	return animations.GetAnimation(animSource);
}

AnimatorId AnimationSystem::Animators::GetNextAnimatorId() {
	lastId = AnimatorId(lastId.GetValue() + 1);
	// TODO: Check for lastId == Invalid and increment if so.
	return lastId;
}

AnimatorId AnimationSystem::AddAnimator(
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
			startSetting.m_RepeatSetting,
			AnimatorAltViewState(animations.GetViewCount(startSetting.m_AnimationId) - 1));

	animators.hotStates.emplace_back(
		newAnimatorId,
		animations.GetTime(startSetting.m_AnimationId, 0));

	// Update target.
	target.rect = animations.GetRect(startSetting.m_AnimationId, 0, 0);

	animationReferenceCounts[startSetting.m_AnimationId]++;

	return newAnimatorId;
}

bool AnimationSystem::RemoveAnimator(const AnimatorId id)
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

void AnimationSystem::AnimatorGui(const AnimatorId id)
{
	if (!AnimatorExists(id)) {
		ImGui::Text("Animator #%u does not exist", id);
		return;
	}

	const AnimatorState& animator = animators.states[id];

	ImGui::Text("Animation:  \t#%u", animator.currentAnimation);
	ImGui::Text("Target Rect:\t0x%08X", animator.target);

	{
		const int numFrames = (int)GetAnimationNumFrames(animator.currentAnimation);
		int frameIndex = (int)animator.currentFrame;
		if (ImGui::SliderInt("Current Frame", &frameIndex, 0, numFrames - 1))
			SetAnimatorFrame(id, frameIndex);
	}
}

bool AnimationSystem::ClearAnimationQueue(const AnimatorId id)
{
	if (!AnimatorExists(id)) return false;

	AnimatorState& animator = animators.states[id];

	animator.queuedAnimations.clear();

	return true;
}

bool AnimationSystem::SetAnimatorAnimation(const AnimatorId animatorId, const AnimatorStartSetting& startSetting, const bool clearQueue)
{
	if (!AnimatorExists(animatorId)) return false;
	if (!AnimationExists(startSetting.m_AnimationId)) return false;

	AnimatorState& animator = animators.states[animatorId];

	// Decrease refcount on current animation.
	animationReferenceCounts[animator.currentAnimation]--;
	// Increase refcount on new animation.
	animationReferenceCounts[startSetting.m_AnimationId]++;

	animator.currentAnimation = startSetting.m_AnimationId;
	animator.currentFrame = 0;
	animator.altViewState =
		AnimatorAltViewState(animations.GetViewCount(animator.currentAnimation) - 1);
	animator.repeatCount = 0;
	animator.repeatSetting = startSetting.m_RepeatSetting;

	if (clearQueue) {
		animator.queuedAnimations.clear();
	}

	// Don't worry about alt views when doing this, the user will call the function
	// to update them at some point.
	animator.target->rect = 
		animations.GetRect(
			animator.currentAnimation, 
			animator.currentFrame);

	animators.hotStates[animator.index].timeLeftInFrame
		= animations.GetTime(
			animator.currentAnimation,
			animator.currentFrame);

	return true;
}

bool AnimationSystem::SetAnimatorTarget(const AnimatorId id, AnimatorTarget & newTarget)
{
	if (!AnimatorExists(id)) return false;

	AnimatorState& animator = animators.states[id];

	animator.target = &newTarget;

	return true;
}

bool AnimationSystem::QueueAnimation(const AnimatorId id, const AnimatorStartSetting& pendingAnimation)
{
	auto log = spdlog::get("console");
	assert(log);

	const auto logCtx = "AnimationSystem::QueueAnimation:";

	if (!AnimatorExists(id)) {
		log->error("{} Animator {} does not exist.", logCtx, id);
		return false;
	}

	animators.states[id].queuedAnimations.push_back(
		pendingAnimation);

	return true;
}

AnimationId AnimationSystem::GetAnimatorAnimation(const AnimatorId animatorId) const
{
	assert(AnimatorExists(animatorId));
	return animators.states.at(animatorId).currentAnimation;
}

unsigned AnimationSystem::GetAnimatorFrame(const AnimatorId animatorId) const
{
	assert(AnimatorExists(animatorId));
	return animators.states.at(animatorId).currentFrame;
}

bool AnimationSystem::SetAnimatorFrame(
	const AnimatorId id,
	const unsigned frameIndex)
{
	if (!AnimatorExists(id)) return false;

	AnimatorState& animator = animators.states[id];

	if (frameIndex >= GetAnimationNumFrames(animator.currentAnimation))
		return false;

	animator.currentFrame = frameIndex;

	animator.target->rect = 
		animations.GetRect(
			animator.currentAnimation, 
			animator.currentFrame, 
			animator.altViewState.currentAltView);

	animators.hotStates[animator.index].timeLeftInFrame =
		animations.GetTime(
			animator.currentAnimation, 
			animator.currentFrame);

	return true;
}

using namespace std::chrono_literals;

void AnimationSystem::Animate(const AnimationSystem::TimeUnit ms) {
	// because this system doesn't support rewinding.
	assert(ms >= TimeUnit::zero());

	for (auto& animatorHotState : animators.hotStates) {
		animatorHotState.timeLeftInFrame -= ms;
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

						if (!SetAnimatorAnimation(
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
		animator.target->rect = 
			animations.GetRect(
				animator.currentAnimation,
				animator.currentFrame,
				animator.altViewState.currentAltView);

		animators.hotStates[animator.index].timeLeftInFrame += 
			animations.GetTime(animator.currentAnimation, animator.currentFrame);
	}

	for (auto animatorId : animatorsToRemove) {
		RemoveAnimator(animatorId);
	}
}

AnimationId AddAnimationFromJson(
	AnimationSystem& animSystem, 
	const nlohmann::json& j, 
	const AnimationSourceInfo& sourceInfo)
{
	static const char* logCtx = "AddAnimationFromJson: ";

	// TODO: Restructure for early-out.

	if (const auto anim = AnimationData::FromJson(j)) {
		if ((*anim).IsValid()) {
			auto ret = animSystem.AddAnimation(*anim, sourceInfo);
			if (ret != AnimationId::Invalid) {
				std::cout << logCtx << "Successfully added animation from JSON to the system with ID " <<
					ret.GetValue() << ".\n";
				return GenerateAnimationId(*anim);
			}
			else {
				std::cout << logCtx << "Could not add animation data from JSON to the system.\n";
				return AnimationId::Invalid;
			}
		}
		else {
			std::cout << logCtx << "Animation data from JSON is not valid.\n";
			return AnimationId::Invalid;
		}
	}
	else {
		std::cout << logCtx << "Could not deserialize animation from JSON.\n";
		return AnimationId::Invalid;
	}
}

void AnimationSystem::UpdateAnimatorAltView(
	const AnimatorId animatorId,
	const float objectAngle,
	const float viewAngle)
{
	const float Tau = 6.28318530718f;

	AnimatorState& animator =
		animators.states.at(animatorId);
	AnimatorAltViewState& altViewState = animator.altViewState;

	// Yup! This is it!
	const unsigned viewIndex =
		(unsigned)((fmodf((objectAngle - viewAngle + Tau), Tau) / Tau)
			* (altViewState.NumAltViews() + 1));

	// Make sure my maths is right.
	assert(viewIndex >= 0);
	assert(viewIndex <= altViewState.NumAltViews());

	if (viewIndex != altViewState.currentAltView) {
		// Update the Animator.
		altViewState.currentAltView = viewIndex;

		animator.target->rect =
			animations.GetRect(
				animator.currentAnimation,
				animator.currentFrame,
				viewIndex);
	}
}

void GuiControls(AnimationSystem& animationSystem, AnimationSystemEditorData& editorData)
{
	ImGui::Text("Num. Animations: %u", animationSystem.GetAnimationCount());
	ImGui::Text("Num. Animators:  %u", animationSystem.GetAnimatorCount());

	if (ImGui::CollapsingHeader("View Animations"))
	{
		ImGui::AutoIndent indent;

		if (animationSystem.GetAnimationCount() == 0) {
			ImGui::Text("No Animations to display");
		}

		std::vector<std::string> animationUidStrings;
		for (auto animId : animationSystem.GetAnimationIds()) {
			animationUidStrings.emplace_back(fmt::format("UID: {}", animId.GetValue()));
		}

		if (editorData.mCurrentSelection > (int)(animationSystem.GetAnimationCount()) - 1) {
			editorData.mCurrentSelection = -1;
		}

		auto itemsGetter = [](void* data, int idx, const char** out_text) -> bool
		{
			auto container = static_cast<std::vector<std::string>*>(data);

			*out_text = (*container)[idx].c_str();

			return true;
		};

		ImGui::Combo("List", &editorData.mCurrentSelection, itemsGetter, (void*)&animationUidStrings, animationUidStrings.size());

		if (editorData.mCurrentSelection > -1) {
			const AnimationId animId = animationSystem.GetAnimationIds()[editorData.mCurrentSelection];

			AnimationSourceInfo animSourceInfo;
			animationSystem.GetAnimationSourceInfo(animId, animSourceInfo);

			ImGui::Text("Name: %s", animSourceInfo.name.c_str());
			ImGui::Text("File: %s", animSourceInfo.filename.c_str());
			ImGui::Text("Ref Count: %d", animationSystem.GetReferenceCount(animId));

			if (ImGui::Button("Remove")) {
				animationSystem.RemoveAnimation(animId);
			}
		}
	}

	if (ImGui::CollapsingHeader("Add Animations")) 
	{
		using namespace JsonHelp;
		
		ImGui::AutoIndent indent;

		ImGui::InputText("JSON Filename", editorData.mFilenameBuffer);

		if (ImGui::Button("Load Individual") && 
			strlen(editorData.mFilenameBuffer)) 
		{

			const nlohmann::json j = LoadJsonFromFile(editorData.mFilenameBuffer);

			AddAnimationFromJson(
				animationSystem, 
				j, 
				AnimationSourceInfo{ {}, editorData.mFilenameBuffer });
		}

		if (ImGui::Button("Load Collection") && 
			strlen(editorData.mFilenameBuffer)) 
		{
			const nlohmann::json j = LoadJsonFromFile(editorData.mFilenameBuffer);

			auto animCollection = j.get<std::map<std::string, nlohmann::json>>();

			for (auto& kvp : animCollection) 
			{
				AddAnimationFromJson(
					animationSystem, 
					kvp.second, 
					AnimationSourceInfo{ kvp.first, editorData.mFilenameBuffer});
			}
		}
	}
}

}