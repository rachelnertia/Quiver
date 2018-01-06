#pragma once

#include <unordered_map>
#include <vector>

#include <gsl/span>
#include <json.hpp>
#include <optional.hpp>

#include "Quiver/Animation/AnimationData.h"
#include "Quiver/Animation/AnimationId.h"
#include "Quiver/Animation/Rect.h"
#include "Quiver/Animation/TimeUnit.h"

namespace qvr
{

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

	auto GetIds() const -> std::vector<AnimationId>;

	friend void to_json(nlohmann::json& j, const AnimationLibrary& animations);

private:
	struct AnimationInfo {
		AnimationInfo(
			const unsigned indexOfFirstRect,
			const unsigned indexOfFirstTime,
			const unsigned numRects,
			const unsigned numRectsPerFrame)
			: mIndexOfFirstRect(indexOfFirstRect)
			, mIndexOfFirstTime(indexOfFirstTime)
			, mNumRects(numRects)
			, mNumRectsPerFrame(numRectsPerFrame)
		{}

		AnimationInfo(const AnimationInfo& other) = default;

		AnimationInfo() = default;

		unsigned NumFrames() const { return mNumRects / mNumRectsPerFrame; }

		std::experimental::optional<AnimationSourceInfo> mSourceInfo;
	
		unsigned mIndexOfFirstRect = 0;
		unsigned mIndexOfFirstTime = 0;
		unsigned mNumRects = 0;
		unsigned mNumRectsPerFrame = 0;
	};

	std::unordered_map<AnimationId, AnimationInfo> infos;
	
	// Time values for each frame in every animation.
	std::vector<Animation::TimeUnit> allFrameTimes;

	// Rects for each frame in every animation, including alt view rects.
	std::vector<Animation::Rect> allFrameRects; 
};

void to_json(nlohmann::json& j, const AnimationLibrary& animations);
void to_json(nlohmann::json& j, const AnimationSourceInfo& sourceInfo);

void from_json(const nlohmann::json& j, AnimationLibrary& animations);
void from_json(const nlohmann::json& j, AnimationSourceInfo& animationSource);

}