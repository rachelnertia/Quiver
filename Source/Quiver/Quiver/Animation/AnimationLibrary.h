#pragma once

#include <chrono>
#include <unordered_map>
#include <vector>

#include <gsl/span>
#include <json.hpp>
#include <optional.hpp>

#include "Quiver/Animation/AnimationData.h"
#include "Quiver/Animation/AnimationId.h"
#include "Quiver/Animation/Rect.h"

namespace qvr
{

namespace Animation
{

using TimeUnit = std::chrono::duration<int, std::milli>;

}

// Tells us about where to find an Animation on disk.
struct AnimationSourceInfo {
	std::string name;
	std::string filename;
};

inline bool operator==(const AnimationSourceInfo& a, const AnimationSourceInfo& b) {
	return a.name == b.name && a.filename == b.filename;
}

class AnimationLibrary
{
public:
	AnimationId Add(const AnimationData& anim);
	AnimationId Add(
		const AnimationData& anim, 
		const AnimationSourceInfo& sourceInfo);

	bool Remove(const AnimationId anim);

	bool Contains(const AnimationId anim) const;

	auto GetCount() const -> int;

	auto GetAnimation(const AnimationSourceInfo& sourceInfo) 
		const -> AnimationId;

	auto GetSourceInfo(const AnimationId anim) 
		const -> std::experimental::optional<AnimationSourceInfo>;

	auto GetFrameCount(const AnimationId anim) const -> int;
	auto GetViewCount(const AnimationId anim) const -> int;
	auto HasAltViews(const AnimationId anim) const -> bool;
	
	auto GetRect(
		const AnimationId anim, 
		const int frameIndex, 
		const int viewIndex = 0) 
			const -> Animation::Rect;

	auto GetRects(
		const AnimationId anim,
		const int frameIndex)
			const -> gsl::span<const Animation::Rect>;

	auto GetTime(
		const AnimationId anim,
		const int frameIndex) 
			const -> Animation::TimeUnit;

	auto GetIds() const -> std::vector<AnimationId> {
		return allIds;
	}

	friend nlohmann::json ToJson(const AnimationLibrary& animations);

private:
	struct AnimationInfo {
		AnimationInfo(
			const unsigned indexOfFirstRect,
			const unsigned indexOfFirstTime,
			const unsigned numRects,
			const unsigned numAltViewsPerFrame)
			: mIndexOfFirstRect(indexOfFirstRect)
			, mIndexOfFirstTime(indexOfFirstTime)
			, mNumRects(numRects)
			, mNumAltViewsPerFrame(numAltViewsPerFrame)
		{}

		AnimationInfo() = default;

		unsigned IndexOfFirstRect() const { return mIndexOfFirstRect; }
		unsigned IndexOfFirstTime() const { return mIndexOfFirstTime; }
		unsigned NumFrames() const { return mNumRects / (mNumAltViewsPerFrame + 1); }
		unsigned NumRects()  const { return mNumRects; }
		unsigned NumTimes()  const { return NumFrames(); }
		unsigned NumRectsPerTime() const { return NumRects() / NumTimes(); }
		unsigned NumAltViewsPerFrame() const { return mNumAltViewsPerFrame; }

	private:
		unsigned mIndexOfFirstRect = 0;
		unsigned mIndexOfFirstTime = 0;
		unsigned mNumRects = 0;
		unsigned mNumAltViewsPerFrame = 0;
	};

	int count = 0;
	std::vector<AnimationId> allIds;
	std::unordered_map<AnimationId, AnimationInfo> infosById;
	std::vector<Animation::TimeUnit> allFrameTimes;
	std::vector<Animation::Rect> allFrameRects; // Rects for each frame in every animation plus their alt view views.
	std::unordered_map<AnimationId, AnimationSourceInfo> sourcesById;
};

auto ToJson(const AnimationLibrary& animations) -> nlohmann::json;

void to_json(nlohmann::json& j, const AnimationSourceInfo& sourceInfo);

void from_json(const nlohmann::json& j, AnimationLibrary& animations);
void from_json(const nlohmann::json& j, AnimationSourceInfo& animationSource);

}