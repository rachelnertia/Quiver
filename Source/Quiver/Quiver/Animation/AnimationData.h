#pragma once

#include <chrono>
#include <optional>

#include <json.hpp>

#include "Quiver/Animation/AnimationId.h"
#include "Quiver/Animation/Rect.h"
#include "Quiver/Animation/TimeUnit.h"

namespace qvr {

namespace Animation {

struct Frame;

}

class AnimationData {
public:
	friend AnimationId GenerateAnimationId(const AnimationData& animation);

	AnimationData() = default;

	static std::optional<AnimationData> FromJson(const nlohmann::json& j);
	static std::optional<AnimationData> FromJsonFile(const std::string filename);

	nlohmann::json ToJson() const;

	void Clear();

	bool IsValid() const;

	int GetFrameCount()       const { return mFrameTimes.size(); }
	int GetRectCount()        const { return mFrameRects.size(); }
	int GetAltViewsPerFrame() const { return mAltViewsPerFrame; }

	std::optional<Animation::Frame>    GetFrame(const int frameIndex)                         const;
	std::optional<Animation::Rect>     GetRect(const int frameIndex, const int viewIndex = 0) const;
	std::optional<Animation::TimeUnit> GetTime(const int frameIndex)                          const;

	std::vector<Animation::TimeUnit> GetTimes() const;
	std::vector<Animation::Rect>     GetRects() const;

	bool AddFrame(const Animation::Frame& frame);

	bool InsertFrame(
		const Animation::Frame& frame,
		const int frameIndex);

	bool AddFrames(const std::vector<Animation::Frame>& frames);

	bool RemoveFrame(const int index);

	bool SetFrame(const int index, const Animation::Frame& frame);
	bool SetFrameTime(const int index, const Animation::TimeUnit time);
	bool SetFrameRect(const int frameIndex, const int viewIndex, const Animation::Rect& rect);

	bool SwapFrames(const int index1, const int index2);

private:
	std::vector<Animation::Rect>     mFrameRects;
	std::vector<Animation::TimeUnit> mFrameTimes;

	unsigned mAltViewsPerFrame = 0;

};

namespace Animation {

struct Frame {
	TimeUnit mTime;
	Rect mBaseRect;
	std::vector<Rect> mAltRects;
};

inline bool operator==(const Frame& lhs, const Frame& rhs) {
	if (lhs.mTime != rhs.mTime)     return false;
	if (lhs.mBaseRect != rhs.mBaseRect) return false;
	if (lhs.mAltRects != rhs.mAltRects) return false;
	return true;
}

inline bool operator!=(const Frame& lhs, const Frame& rhs) {
	return !(lhs == rhs);
}

}

}