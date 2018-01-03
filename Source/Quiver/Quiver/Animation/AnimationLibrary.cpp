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
	if (infosById.find(id) != infosById.end()) {
		return id;
	}

	infosById[id] =
		AnimationInfo(
			allFrameRects.size(),
			allFrameTimes.size(),
			anim.GetRectCount(),
			anim.GetAltViewsPerFrame());

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
		infosById[newAnim].mSourceInfo = sourceInfo;
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

	AnimationInfo animInfo = infosById[anim];

	log->debug("{} Animation found.", logCtx);

	infosById.erase(anim);

	// Erase views.
	{
		auto start = allFrameRects.begin() + animInfo.IndexOfFirstRect();
		auto end = start + animInfo.NumRects();
		allFrameRects.erase(start, end);
	}

	// Erase times.
	{
		auto start = allFrameTimes.begin() + animInfo.IndexOfFirstTime();
		auto end = start + animInfo.NumTimes();
		allFrameTimes.erase(start, end);
	}

	// Loop through the remaining AnimationInfos and fix up the indexOfFirstRect members
	// of those whose indexOfFirstRect was greater than this one's
	// TODO: This smells!
	for (auto& kvp : infosById) {
		if (kvp.second.IndexOfFirstRect() > animInfo.IndexOfFirstRect()) {
			AnimationInfo newAnimInfo(
				kvp.second.IndexOfFirstRect() - animInfo.NumRects(),
				kvp.second.IndexOfFirstTime(),
				kvp.second.NumRects(),
				kvp.second.NumAltViewsPerFrame());

			infosById.insert_or_assign(kvp.first, newAnimInfo);
		}
	}

	// And repeatSetting for times.
	// TODO: This smells!
	for (auto& kvp : infosById) {
		if (kvp.second.IndexOfFirstTime() > animInfo.IndexOfFirstTime()) {
			AnimationInfo newAnimInfo(
				kvp.second.IndexOfFirstRect(),
				kvp.second.IndexOfFirstTime() - animInfo.NumTimes(),
				kvp.second.NumRects(),
				kvp.second.NumAltViewsPerFrame());

			infosById.insert_or_assign(kvp.first, newAnimInfo);
		}
	}

	log->debug("{} Successfully removed animation. Remaining: {}", logCtx, GetCount());

	return true;
}

bool AnimationLibrary::Contains(const AnimationId anim) const
{
	return this->infosById.count(anim) == 1;
}

int AnimationLibrary::GetCount() const
{
	return this->infosById.size();
}

AnimationId AnimationLibrary::GetAnimation(const AnimationSourceInfo& sourceInfo) const
{
	for (const auto& kvp : infosById) 
	{
		if (kvp.second.mSourceInfo && 
			kvp.second.mSourceInfo.value() == sourceInfo) 
		{
			return kvp.first;
		}
	}

	return AnimationId::Invalid;
}

auto AnimationLibrary::GetSourceInfo(const AnimationId anim) const -> std::experimental::optional<AnimationSourceInfo>
{
	if (Contains(anim)) {
		const auto it = infosById.find(anim);

		if (it != infosById.end()) {
			return it->second.mSourceInfo;
		}
	}

	return {};
}

int AnimationLibrary::GetFrameCount(const AnimationId anim) const
{
	return infosById.at(anim).NumFrames();
}

bool AnimationLibrary::HasAltViews(const AnimationId anim) const
{
	return GetViewCount(anim) > 1;
}

int AnimationLibrary::GetViewCount(const AnimationId anim) const
{
	return infosById.at(anim).NumAltViewsPerFrame() + 1;
}

auto AnimationLibrary::GetRect(
	const AnimationId anim,
	const int frameIndex,
	const int viewIndex)
		const -> Animation::Rect
{
	const AnimationInfo info = infosById.at(anim);

	const int rectIndex = (frameIndex * (info.NumAltViewsPerFrame() + 1)) + viewIndex;

	return allFrameRects[info.IndexOfFirstRect() + rectIndex];
}

auto AnimationLibrary::GetRects(
	const AnimationId anim,
	const int frameIndex)
		const -> gsl::span<const Animation::Rect>
{
	const AnimationInfo info = infosById.at(anim);

	const int firstRectIndex = (frameIndex * (info.NumRectsPerTime()));

	return gsl::make_span(
		&allFrameRects[info.IndexOfFirstRect() + firstRectIndex],
		info.NumRectsPerTime());
}

auto AnimationLibrary::GetTime(
	const AnimationId anim,
	const int frameIndex)
		const -> Animation::TimeUnit
{
	const AnimationInfo info = infosById.at(anim);

	return allFrameTimes[info.IndexOfFirstTime() + frameIndex];
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
	return ExtractKeys(infosById);
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
	animationSource.filename = j.at("File").get<std::string>();
	animationSource.name     = j.value<std::string>("Name", {});
}

void to_json(nlohmann::json& j, const AnimationLibrary& animations)
{
	for (auto kvp : animations.infosById)
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