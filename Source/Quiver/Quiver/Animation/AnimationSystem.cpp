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

AnimationId AddAnimationFromJson(AnimationSystem& animSystem, const nlohmann::json& j);

static constexpr const char* AnimationSourcesFieldTitle = "AnimationSources";

bool AnimationSystem::FromJson(const nlohmann::json & j)
{
	static const std::string logCtx = "AnimationSystem::FromJson: ";

	if (!j.is_object()) {
		std::cout << logCtx << "Error: json object is not an object.\n";
		return false;
	}

	if (j.find(AnimationSourcesFieldTitle) == j.end()) {
		std::cout << logCtx << "Error: json object does not contain a field named \"SourceFiles\".\n";
		return false;
	}

	if (!j[AnimationSourcesFieldTitle].is_array()) {
		std::cout << logCtx << "Error: \"" << AnimationSourcesFieldTitle <<
			"\" is not an array.\n";
		return false;
	}

	std::vector<AnimationSourceInfo> animSources;

	int totalCount = j[AnimationSourcesFieldTitle].size();
	int validCount = 0;
	int currentIndex = -1;
	int fromCollectionCount = 0;
	for (const nlohmann::json& sourceInfoJson : j[AnimationSourcesFieldTitle]) {
		currentIndex++;

		if (!sourceInfoJson.is_object()) {
			std::cout << logCtx << "Error: Array entry " << currentIndex << " is not an object.\n";
			continue;
		}

		if (sourceInfoJson.find("File") == sourceInfoJson.end()) {
			std::cout << logCtx << "Error: Array entry " << currentIndex << " does not contain a \"File\" field.\n";
			continue;
		}

		if (!sourceInfoJson["File"].is_string()) {
			std::cout << logCtx << "Error: Array entry " << currentIndex << "'s \"File\" field is not a string.\n";
			continue;
		}

		bool fromCollection = false;

		if (sourceInfoJson.find("Name") != sourceInfoJson.end()) {
			if (!sourceInfoJson["Name"].is_string()) {
				std::cout << logCtx << "Error: Array entry " << currentIndex << " has a \"Name\" field but "
					"it is not a string.\n";
				continue;
			}

			fromCollection = true;
			fromCollectionCount++;
		}

		AnimationSourceInfo animSourceInfo;
		animSourceInfo.filename = sourceInfoJson["File"].get<std::string>();
		if (fromCollection) {
			animSourceInfo.name = sourceInfoJson["Name"].get<std::string>();
		}

		validCount++;

		animSources.push_back(animSourceInfo);
	}

	std::cout << logCtx << "Found " << totalCount << " sources (" <<
		validCount << " valid).\n";

	std::cout << logCtx << fromCollectionCount << " are from collections.\n";

	int successCount = 0;
	for (auto& animSourceInfo : animSources) {
		std::ifstream file(animSourceInfo.filename);
		if (!file.is_open()) {
			std::cout << logCtx << "Error: Could not open \"" <<
				animSourceInfo.filename << "\".\n";
			continue;
		}

		nlohmann::json fileJson;

		fileJson << file;

		if (animSourceInfo.name.empty()) {
			// Load individual file.
			if (const auto animData = AnimationData::FromJson(fileJson)) {
				AnimationId animId = AddAnimation(*animData);

				if (animId == AnimationId::Invalid) {
					std::cout << logCtx << "Error: Could not add animation loaded from \""
						<< animSourceInfo.filename << "\" to system.\n";
					continue;
				}

				animations.sourcesById[animId] = animSourceInfo;

				std::cout << logCtx << "Loaded animation from \"" << animSourceInfo.filename <<
					"\", giving animation ID " << animId.GetValue() << ".\n";
			}
			else {
				std::cout << "Error: Could not deserialize an AnimationData from \""
					<< animSourceInfo.filename << "\".\n";
				continue;
			}
		}
		else {
			// Load from collection file.
			if (fileJson.find(animSourceInfo.name) == fileJson.end()) {
				std::cout << logCtx << "Error: Could not find \"" << animSourceInfo.name << "\" in file \"" <<
					animSourceInfo.filename << "\".\n";
				continue;
			}

			if (const auto animData = AnimationData::FromJson(fileJson[animSourceInfo.name])) {
				const AnimationId animId = AddAnimation(*animData);

				if (animId == AnimationId::Invalid) {
					std::cout << logCtx << "Error: Could not add animation loaded from field \"" <<
						animSourceInfo.name << "\" in collection file \"" <<
						animSourceInfo.filename << "\".\n";
					continue;
				}

				animations.sourcesById[animId] = animSourceInfo;

				std::cout << logCtx << "Loaded animation from field \"" <<
					animSourceInfo.name << "\" in collection file \"" <<
					animSourceInfo.filename << "\", giving animation ID "
					<< animId.GetValue() << ".\n";
			}
			else {
				std::cout << logCtx << "Error: Could not deserialize an AnimationData from field \""
					<< animSourceInfo.name << "\" in collection file \"" <<
					animSourceInfo.filename << "\".\n";
				continue;
			}
		}

		successCount++;
	}

	std::cout << logCtx << "Successfully loaded " << successCount << " animations!\n";

	return true;
}

// Store the source info of every Animation currently in the System so that we can reload later.
nlohmann::json AnimationSystem::ToJson() const
{
	static const std::string logCtx = "AnimationSystem::ToJson: ";

	std::cout << logCtx << "Serializing info about " << animations.count << " animations...\n";

	nlohmann::json j;

	int count = 0;
	for (auto kvp : animations.sourcesById)
	{
		nlohmann::json animSourceJson;
		animSourceJson["File"] = kvp.second.filename;
		if (!kvp.second.name.empty()) {
			animSourceJson["Name"] = kvp.second.name;
		}
		j[AnimationSourcesFieldTitle][count] = animSourceJson;
		count++;
	}

	return j;
}

AnimationId AnimationSystem::AddAnimation(const AnimationData & anim) {
	if (!anim.IsValid()) {
		std::cout << "AnimationSystem::AddAnimation: AnimationData is invalid.\n";
		return AnimationId::Invalid;
	}

	// generate an AnimationId
	const AnimationId id = GenerateAnimationId(anim);

	if (id == AnimationId::Invalid) return AnimationId::Invalid;

	// an animation with the given id already exists
	if (animations.infosById.find(id) != animations.infosById.end()) {
		return id;
	}

	animations.infosById[id] =
		AnimationInfo(
			animations.allFrameRects.size(),
			animations.allFrameTimes.size(),
			anim.GetRectCount(),
			anim.GetAltViewsPerFrame());

	// Pack the animation's frame rects and times into the arrays.
	const auto frameTimes = anim.GetTimes();
	animations.allFrameTimes.insert(
		animations.allFrameTimes.end(),
		frameTimes.begin(),
		frameTimes.end());
	const auto frameRects = anim.GetRects();
	animations.allFrameRects.insert(
		animations.allFrameRects.end(),
		frameRects.begin(),
		frameRects.end());

	assert(animations.referenceCountsById.count(id) == 0);

	animations.referenceCountsById[id] = 0;

	animations.allIds.push_back(id);

	animations.count++;

	return id;
}

AnimationId AnimationSystem::AddAnimation(const AnimationData& data, const AnimationSourceInfo& sourceInfo)
{
	const AnimationId newAnim = AddAnimation(data);

	if (newAnim != AnimationId::Invalid) 
	{
		animations.sourcesById[newAnim] = sourceInfo;
	}

	return newAnim;
}

bool AnimationSystem::RemoveAnimation(const AnimationId id)
{
	std::stringstream logCtx;
	logCtx << "AnimationSystem::RemoveAnimation(" << id.GetValue() << "): ";

	if (!AnimationExists(id)) {
		std::cout << logCtx.str() << "Animation does not exist.\n";
		return false;
	}

	AnimationInfo animInfo = animations.infosById[id];

	std::cout << logCtx.str()
		<< "Animation found. "
		<< "\n Start Frame Index: " << animInfo.IndexOfFirstRect()
		<< "\n Num. Frames: " << animInfo.NumFrames()
		<< "\n Num. Rects: " << animInfo.NumRects()
		<< "\n Num. Times: " << animInfo.NumTimes()
		<< "\n Num. Alt. Rects Per Frame: " << animInfo.NumAltViewsPerFrame()
		<< "\n Ref. Count: " << animations.referenceCountsById[id]
		<< "\n";

	animations.infosById.erase(id);

	// Erase rects.
	{
		auto start = animations.allFrameRects.begin() + animInfo.IndexOfFirstRect();
		auto end = start + animInfo.NumRects();
		animations.allFrameRects.erase(start, end);
	}

	// Erase times.
	{
		auto start = animations.allFrameTimes.begin() + animInfo.IndexOfFirstTime();
		auto end = start + animInfo.NumTimes();
		animations.allFrameTimes.erase(start, end);
	}

	// Loop through the remaining AnimationInfos and fix up the indexOfFirstRect members
	// of those whose indexOfFirstRect was greater than this one's
	// TODO: This smells!
	for (auto& kvp : animations.infosById) {
		if (kvp.second.IndexOfFirstRect() > animInfo.IndexOfFirstRect()) {
			AnimationInfo newAnimInfo(
				kvp.second.IndexOfFirstRect() - animInfo.NumRects(),
				kvp.second.IndexOfFirstTime(),
				kvp.second.NumRects(),
				kvp.second.NumAltViewsPerFrame());

			animations.infosById.insert_or_assign(kvp.first, newAnimInfo);
		}
	}

	// And repeatSetting for times.
	// TODO: This smells!
	for (auto& kvp : animations.infosById) {
		if (kvp.second.IndexOfFirstTime() > animInfo.IndexOfFirstTime()) {
			AnimationInfo newAnimInfo(
				kvp.second.IndexOfFirstRect(),
				kvp.second.IndexOfFirstTime() - animInfo.NumTimes(),
				kvp.second.NumRects(),
				kvp.second.NumAltViewsPerFrame());

			animations.infosById.insert_or_assign(kvp.first, newAnimInfo);
		}
	}

	// Animators that reference this Animation are now invalid, so we need to delete them.
	{
		std::vector<AnimatorId> animatorsToRemove;

		for (const auto& animator : animators.states) {
			if (animator.second.currentAnimation == id) {
				animatorsToRemove.push_back(animator.first);
			}
		}

		std::cout << logCtx.str() << "Found " << animatorsToRemove.size() << " Animators that reference this Animation. Removing them...\n";

		for (auto animatorId : animatorsToRemove) {
			RemoveAnimator(animatorId);
		}
	}

	animations.referenceCountsById.erase(id);

	{
		auto it = std::find(animations.allIds.begin(), animations.allIds.end(), id);
		animations.allIds.erase(it);
	}

	animations.count--;

	// Remove serialization stuff, if it exists:
	if (animations.sourcesById.find(id) != animations.sourcesById.end()) {
		animations.sourcesById.erase(id);
	}

	std::cout << logCtx.str() << "Success! Num. Animations Remaining: " << animations.count << "\n";

	return true;
}

bool AnimationSystem::GetAnimationSourceInfo(const AnimationId id, AnimationSourceInfo & animSource) const
{
	const auto it = animations.sourcesById.find(id);

	if (it == animations.sourcesById.end()) {
		return false;
	}

	animSource = it->second;

	return true;
}

unsigned AnimationSystem::GetAnimationNumFrames(const AnimationId id) const
{
	const AnimationInfo& animInfo = animations.infosById.at(id);
	return animInfo.NumFrames();
}

bool AnimationSystem::AnimationHasAltViews(const AnimationId id) const
{
	assert(AnimationExists(id));

	if (animations.infosById.count(id) == 0) return false;

	return animations.infosById.at(id).NumAltViewsPerFrame() > 0;
}

AnimationId AnimationSystem::GetAnimationFromSource(const AnimationSourceInfo & animSource) const
{
	for (const auto& kvp : animations.sourcesById) {
		if (kvp.second == animSource) {
			return kvp.first;
		}
	}

	return AnimationId::Invalid;
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
	if (startSetting.m_AnimationId == AnimationId::Invalid) {
		return AnimatorId::Invalid;
	}
	if (animations.infosById.find(startSetting.m_AnimationId) == animations.infosById.end()) {
		return AnimatorId::Invalid;
	}

	const AnimationInfo animInfo =
		animations.infosById[startSetting.m_AnimationId];

	const int frameIndex = 0;

	const AnimatorId newAnimatorId = animators.GetNextAnimatorId();

	// Initialise Animator's time left in frame.
	const int timeIndex = animators.hotStates.size();
	animators.hotStates.emplace_back(
		newAnimatorId,
		animations.allFrameTimes[
			animInfo.IndexOfFirstTime() + frameIndex]);

	animators.states[newAnimatorId] =
		AnimatorState(
			startSetting.m_AnimationId,
			frameIndex,
			timeIndex,
			target,
			startSetting.m_RepeatSetting,
			AnimatorAltViewState(animInfo.NumAltViewsPerFrame()));

	// Update target.
	target.rect = animations.allFrameRects[
		animInfo.IndexOfFirstRect() + (frameIndex * animInfo.NumRectsPerTime())];

	animations.referenceCountsById[startSetting.m_AnimationId]++;

	return newAnimatorId;
}

bool AnimationSystem::RemoveAnimator(const AnimatorId id)
{
	assert(id != AnimatorId::Invalid);

	// TODO: Add logging.

	if (animators.states.count(id) == 0) return false;

	{
		const AnimatorState& animator = animators.states[id];

		animations.referenceCountsById[animator.currentAnimation]--;

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
	animations.referenceCountsById[animator.currentAnimation]--;
	// Increase refcount on new animation.
	animations.referenceCountsById[startSetting.m_AnimationId]++;

	const AnimationInfo& animInfo =
		animations.infosById[startSetting.m_AnimationId];

	const int firstFrameIndex = 0;

	// Don't worry about alt views when doing this, the user will call the function
	// to update them at some point.
	const auto firstFrameRect =
		animations.allFrameRects[
			animInfo.IndexOfFirstRect()
				+ (firstFrameIndex * animInfo.NumRectsPerTime())];
	const auto firstFrameTime =
		animations.allFrameTimes[
			animInfo.IndexOfFirstTime() + firstFrameIndex];

	animator.currentAnimation = startSetting.m_AnimationId;
	animator.currentFrame = firstFrameIndex;
	animator.altViewState =
		AnimatorAltViewState(animInfo.NumAltViewsPerFrame());
	animator.repeatCount = 0;
	animator.repeatSetting = startSetting.m_RepeatSetting;

	if (clearQueue) {
		animator.queuedAnimations.clear();
	}

	animator.target->rect = firstFrameRect;

	animators.hotStates[animator.index].timeLeftInFrame
		= firstFrameTime;

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

unsigned AnimationSystem::CalculateRectIndex(
	const AnimationSystem::AnimationInfo& animationInfo,
	const unsigned frameIndexInAnimation,
	const unsigned currentAltView)
{
	assert(frameIndexInAnimation < animationInfo.NumFrames());
	assert(currentAltView < animationInfo.NumRectsPerTime());

	return animationInfo.IndexOfFirstRect()
		+ (frameIndexInAnimation * animationInfo.NumRectsPerTime())
		+ currentAltView;
}

bool AnimationSystem::SetAnimatorFrame(
	const AnimatorId id,
	const unsigned frameIndex)
{
	if (!AnimatorExists(id)) return false;

	AnimatorState& animator = animators.states[id];

	if (frameIndex >= GetAnimationNumFrames(animator.currentAnimation))
		return false;

	const AnimationInfo animationInfo =
		animations.infosById[animator.currentAnimation];

	// Set frame
	animator.currentFrame = frameIndex;

	// Update target
	{
		const auto rectIndex = CalculateRectIndex(
			animationInfo,
			frameIndex,
			animator.altViewState.currentAltView);

		animator.target->rect = animations.allFrameRects[rectIndex];
	}

	// Update time
	animators.hotStates[animator.index].timeLeftInFrame =
		animations.allFrameTimes[animationInfo.IndexOfFirstTime() + frameIndex];

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
		const AnimationInfo animInfo = animations.infosById.at(animator.currentAnimation);

		animator.currentFrame = (animator.currentFrame + 1) % animInfo.NumFrames();

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
		{
			const unsigned nextRectIndex =
				CalculateRectIndex(
					animInfo,
					animator.currentFrame,
					animator.altViewState.currentAltView);

			const Rect nextRect = animations.allFrameRects[nextRectIndex];

			animator.target->rect = nextRect;
		}

		{
			const int timeIndex = animInfo.IndexOfFirstTime() + animator.currentFrame;
			const TimeUnit nextTime = animations.allFrameTimes[timeIndex];
			animators.hotStates[animator.index].timeLeftInFrame += nextTime;
		}
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

void AnimationSystem::UpdateAnimatorAltView(const AnimatorId animatorId,
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

		const AnimationInfo& animInfo =
			animations.infosById.at(animator.currentAnimation);

		// Update the target rect.
		animator.target->rect = animations.allFrameRects[
			animInfo.IndexOfFirstRect() +
				animator.currentFrame +
				viewIndex];
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