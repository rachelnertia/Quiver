#include "AnimationLibrary.h"

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

#include "Quiver/Misc/JsonHelpers.h"

namespace qvr
{

AnimationId AnimationLibrary::Add(const AnimationData& anim)
{
	auto log = spdlog::get("console");
	assert(log);

	if (!anim.IsValid()) {
		log->warn("AnimationLibrary::Add: AnimationData is invalid!");
		return AnimationId::Invalid;
	}

	// generate an AnimationId
	const AnimationId id = GenerateAnimationId(anim);

	if (id == AnimationId::Invalid) return AnimationId::Invalid;

	// an animation with the given id already exists
	if (infos.find(id) != infos.end()) {
		return id;
	}

	infos[id] =
		AnimationInfo(
			allFrameRects.size(),
			allFrameTimes.size(),
			anim.GetRectCount(),
			anim.GetRectCount() / anim.GetFrameCount());

	// Pack the animation's frame views and times into the arrays.
	const auto frameTimes = anim.GetTimes();
	allFrameTimes.insert(
		allFrameTimes.end(),
		frameTimes.begin(),
		frameTimes.end());
	const auto frameRects = anim.GetRects();
	allFrameRects.insert(
		allFrameRects.end(),
		frameRects.begin(),
		frameRects.end());

	return id;
}

AnimationId AnimationLibrary::Add(const AnimationData& anim, const AnimationSourceInfo& sourceInfo)
{
	const AnimationId newAnim = Add(anim);

	if (newAnim != AnimationId::Invalid)
	{
		infos[newAnim].mSourceInfo = sourceInfo;
	}

	return newAnim;
}

bool AnimationLibrary::Remove(const AnimationId anim)
{
	auto log = spdlog::get("console");
	assert(log);

	std::string logCtx = fmt::format("AnimationLibrary::Remove({}): ", anim);

	if (!Contains(anim)) {
		log->warn("{} Animation does not exist!", logCtx);
		return false;
	}

	const AnimationInfo targetAnimInfo = infos[anim];

	log->debug("{} Animation found.", logCtx);

	infos.erase(anim);

	// Erase views.
	{
		auto start = allFrameRects.begin() + targetAnimInfo.mIndexOfFirstRect;
		auto end = start + targetAnimInfo.mNumRects;
		allFrameRects.erase(start, end);
	}

	// Erase times.
	{
		auto start = allFrameTimes.begin() + targetAnimInfo.mIndexOfFirstTime;
		auto end = start + targetAnimInfo.NumFrames();
		allFrameTimes.erase(start, end);
	}

	// Loop through the remaining AnimationInfos and fix up the indexOfFirstRect members
	// of those whose indexOfFirstRect was greater than this one's
	for (auto& kvp : infos) {
		if (kvp.second.mIndexOfFirstRect > targetAnimInfo.mIndexOfFirstRect) {
			AnimationInfo newAnimInfo(kvp.second);

			newAnimInfo.mIndexOfFirstRect -= targetAnimInfo.mNumRects;

			infos[kvp.first] = newAnimInfo;
		}
	}

	// And repeat for times.
	for (auto& kvp : infos) {
		if (kvp.second.mIndexOfFirstTime > targetAnimInfo.mIndexOfFirstTime) {
			AnimationInfo newAnimInfo(kvp.second);

			newAnimInfo.mIndexOfFirstTime -= targetAnimInfo.NumFrames();

			infos[kvp.first] = newAnimInfo;
		}
	}

	log->debug("{} Successfully removed animation. Remaining: {}", logCtx, GetCount());

	return true;
}

bool AnimationLibrary::Contains(const AnimationId anim) const
{
	return this->infos.count(anim) == 1;
}

int AnimationLibrary::GetCount() const
{
	return this->infos.size();
}

AnimationId AnimationLibrary::GetAnimation(const AnimationSourceInfo& sourceInfo) const
{
	for (const auto& kvp : infos) 
	{
		if (kvp.second.mSourceInfo && 
			kvp.second.mSourceInfo.value() == sourceInfo) 
		{
			return kvp.first;
		}
	}

	return AnimationId::Invalid;
}

auto AnimationLibrary::GetSourceInfo(const AnimationId anim) const -> std::optional<AnimationSourceInfo>
{
	if (Contains(anim)) {
		const auto it = infos.find(anim);

		if (it != infos.end()) {
			return it->second.mSourceInfo;
		}
	}

	return {};
}

int AnimationLibrary::GetFrameCount(const AnimationId anim) const
{
	return infos.at(anim).NumFrames();
}

bool AnimationLibrary::HasAltViews(const AnimationId anim) const
{
	return GetViewCount(anim) > 1;
}

int AnimationLibrary::GetViewCount(const AnimationId anim) const
{
	return infos.at(anim).mNumRectsPerFrame;
}

auto AnimationLibrary::GetRect(
	const AnimationId anim,
	const int frameIndex,
	const int viewIndex)
		const -> Animation::Rect
{
	const AnimationInfo info = infos.at(anim);

	const int rectIndex = (frameIndex * info.mNumRectsPerFrame) + viewIndex;

	return allFrameRects[info.mIndexOfFirstRect + rectIndex];
}

auto AnimationLibrary::GetRects(
	const AnimationId anim,
	const int frameIndex)
		const -> gsl::span<const Animation::Rect>
{
	const AnimationInfo info = infos.at(anim);

	const int firstRectIndex = (frameIndex * (info.mNumRectsPerFrame));

	return gsl::make_span(
		&allFrameRects[info.mIndexOfFirstRect + firstRectIndex],
		info.mNumRectsPerFrame);
}

auto AnimationLibrary::GetTime(
	const AnimationId anim,
	const int frameIndex)
		const -> Animation::TimeUnit
{
	const AnimationInfo info = infos.at(anim);

	return allFrameTimes[info.mIndexOfFirstTime + frameIndex];
}

template<typename KeyType, typename ValType>
auto ExtractKeys(const std::unordered_map<KeyType, ValType> map) -> std::vector<KeyType>
{
	std::vector<KeyType> keys;
	
	keys.reserve(map.size());
	
	std::transform(
		std::begin(map),
		std::end(map),
		std::back_inserter(keys),
		[](const std::pair<KeyType, ValType>& kvp) {
			return kvp.first;
		});
	
	return keys;
}

auto AnimationLibrary::GetIds() const -> std::vector<AnimationId> {
	return ExtractKeys(infos);
}

using json = nlohmann::json;

void to_json(json& j, const AnimationSourceInfo& animationSource) {
	j["File"] = animationSource.filename;

	if (!animationSource.name.empty()) {
		j["Name"] = animationSource.name;
	}
}

void from_json(const json& j, AnimationSourceInfo& animationSource)
{
	if (j.empty()) {
		return;
	}
	animationSource.filename = j.at("File").get<std::string>();
	animationSource.name     = j.value<std::string>("Name", {});
}

void to_json(nlohmann::json& j, const AnimationLibrary& animations)
{
	for (auto kvp : animations.infos)
	{
		if (kvp.second.mSourceInfo.has_value() == false) continue;
		
		j.push_back(kvp.second.mSourceInfo.value());
	}
}

bool AddFromSource(AnimationLibrary& library, const AnimationSourceInfo& sourceInfo)
{
	if (sourceInfo.name.empty()) {
		auto animation = AnimationData::FromJsonFile(sourceInfo.filename);

		if (!animation) {
			return false;
		}

		return AnimationId::Invalid != library.Add(animation.value(), sourceInfo);
	}

	json j = JsonHelp::LoadJsonFromFile(sourceInfo.filename);

	// This is exception town...
	return 
		AnimationId::Invalid != library.Add(
			AnimationData::FromJson(j.at(sourceInfo.name)).value(), 
			sourceInfo);
}

void from_json(const json& j, AnimationLibrary& animations)
{
	if (j.is_null()) return;

	const auto animSources = j.get<std::vector<AnimationSourceInfo>>();
	
	for (auto& animSource : animSources) {
		AddFromSource(animations, animSource);
	}
}

}