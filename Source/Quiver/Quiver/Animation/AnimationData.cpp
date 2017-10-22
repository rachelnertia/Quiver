#include "AnimationData.h"

#include <fstream>

#include <optional.hpp>

using namespace std::literals::chrono_literals;

namespace
{

const char* FieldName_AltViewCount = "altViewsPerFrame";
const char* FieldName_FrameRects = "frameRects";
const char* FieldName_FrameTimes = "frameTimes";

}

template <typename T>
using optional = std::experimental::optional<T>;

namespace qvr {

using namespace Animation;

// If two AnimationDatas are the same this function should output the same AnimationId.
// If you find that it doesn't, fix it!
AnimationId GenerateAnimationId(const AnimationData & animation)
{
	unsigned id = 0;
	id += animation.mFrameRects.size();
	id *= (animation.mAltViewsPerFrame + 1);
	for (const auto& frame : animation.mFrameRects) {
		id *= 2;
		id += frame.bottom;
		id += frame.left;
		id += frame.top;
		id += frame.right;
	}
	for (const auto time : animation.mFrameTimes) {
		id *= 2;
		id += time.count();
	}
	return AnimationId(id);
}

optional<AnimationData> AnimationData::FromJson(const nlohmann::json & j) {
	if (!j.is_object())	return {};

	if (j.count(FieldName_FrameRects) == 0) return {};
	if (j.count(FieldName_FrameTimes) == 0) return {};
	if (!j[FieldName_FrameTimes].is_array()) return {};
	if (!j[FieldName_FrameRects].is_array()) return {};

	std::vector<Rect> tempFrameRects;

	for (const auto& rectJson : j[FieldName_FrameRects]) {
		Rect rect;
		if (rect.FromJson(rectJson)) {
			tempFrameRects.push_back(rect);
		}
	}

	std::vector<TimeUnit> tempFrameTimes;
	for (const auto& timeJson : j[FieldName_FrameTimes]) {
		tempFrameTimes.push_back((TimeUnit)timeJson.get<int>());
	}

	const unsigned altViewCount = j.value<unsigned>(FieldName_AltViewCount, 0);

	if (tempFrameRects.size() % (altViewCount + 1) != 0) return {};

	for (const auto time : tempFrameTimes) {
		if (time <= 0ms) return {};
	}

	AnimationData anim;

	anim.mFrameRects = std::move(tempFrameRects);
	anim.mFrameTimes = std::move(tempFrameTimes);
	anim.mAltViewsPerFrame = altViewCount;

	return anim;
}

std::experimental::optional<AnimationData> AnimationData::FromJsonFile(const std::string filename)
{
	std::ifstream file(filename);

	if (!file.is_open())
		return {};

	nlohmann::json j;

	try
	{
		j << file;
	}
	catch (std::invalid_argument exception)
	{
		return {};
	}

	return FromJson(j);
}

using json = nlohmann::json;

json AnimationData::ToJson() const {
	if (!IsValid()) return {};

	json j;

	for (const auto rect : mFrameRects) {
		json rectJson;
		rect.ToJson(rectJson);
		j[FieldName_FrameRects].push_back(rectJson);
	}

	for (const auto time : mFrameTimes) {
		j[FieldName_FrameTimes].push_back(time.count());
	}

	if (mAltViewsPerFrame > 0) {
		j[FieldName_AltViewCount] = mAltViewsPerFrame;
	}

	return j;
}

void AnimationData::Clear() {
	mFrameRects.clear();
	mFrameTimes.clear();
	mAltViewsPerFrame = 0;
}

bool AnimationData::IsValid() const {
	if (mFrameRects.size() <= 1) return false;
	if (mFrameRects.size() / (mAltViewsPerFrame + 1) <= 1) return false;

	if (mFrameTimes.size() <= 1) return false;

	if (mFrameRects.size() / mFrameTimes.size() != (mAltViewsPerFrame + 1)) {
		return false;
	}

	for (const auto frameTime : mFrameTimes) {
		if (frameTime <= 0ms) return false;
	}

	return true;
}

bool FrameIsValid(const AnimationData& animation, const Animation::Frame& frame) {
	if ((int)frame.mAltRects.size() != animation.GetAltViewsPerFrame()) return false;
	if (frame.mTime <= 0ms) return false;

	return true;
}

bool FrameIndexIsValid(const AnimationData& animation, const int index) {
	if (index < 0) return false;
	if (index >= animation.GetFrameCount()) return false;

	return true;
}

bool ViewIndexIsValid(const AnimationData& animation, const int index) {
	if (index < 0) return false;
	if (index > animation.GetAltViewsPerFrame()) return false;

	return true;
}

int GetRectIndex(const AnimationData& animation, const int frameIndex, const int viewIndex) {
	return (frameIndex * (animation.GetAltViewsPerFrame() + 1)) + viewIndex;
}

std::experimental::optional<Animation::Frame> AnimationData::GetFrame(const int frameIndex) const
{
	if (!FrameIndexIsValid(*this, frameIndex)) return {};

	const int firstRectIndex = GetRectIndex(*this, frameIndex, 0);

	std::vector<Rect> altViewRects;

	if (mAltViewsPerFrame > 0) {
		altViewRects.insert(
			altViewRects.end(),
			mFrameRects.begin() + firstRectIndex + 1,
			mFrameRects.begin() + firstRectIndex + mAltViewsPerFrame);
	}

	return Frame{
		mFrameTimes[frameIndex],
		mFrameRects[firstRectIndex],
		std::move(altViewRects)
	};
}

std::experimental::optional<Animation::Rect> AnimationData::GetRect(const int frameIndex, const int viewIndex) const
{
	if (!FrameIndexIsValid(*this, frameIndex)) return {};
	if (!ViewIndexIsValid(*this, viewIndex)) return {};

	const int realIndex = GetRectIndex(*this, frameIndex, viewIndex);

	return mFrameRects[realIndex];
}

std::experimental::optional<AnimationData::TimeUnit> AnimationData::GetTime(const int frameIndex) const
{
	if (!FrameIndexIsValid(*this, frameIndex)) return {};

	return mFrameTimes[frameIndex];
}

std::vector<AnimationData::TimeUnit> AnimationData::GetTimes() const
{
	return mFrameTimes;
}

std::vector<Animation::Rect> AnimationData::GetRects() const
{
	return mFrameRects;
}

bool AnimationData::AddFrame(const Animation::Frame& frame) {
	if (!FrameIsValid(*this, frame)) return false;

	using namespace std;

	mFrameRects.push_back(frame.mBaseRect);
	mFrameRects.insert(end(mFrameRects), begin(frame.mAltRects), end(frame.mAltRects));
	mFrameTimes.push_back(frame.mTime);

	return true;
}

bool AnimationData::InsertFrame(
	const Animation::Frame & frame,
	const int frameIndex)
{
	if (!FrameIndexIsValid(*this, frameIndex)) return false;
	if (!FrameIsValid(*this, frame)) return false;

	mFrameRects.insert(
		mFrameRects.begin() + frameIndex,
		frame.mBaseRect);

	mFrameRects.insert(
		mFrameRects.begin() + frameIndex + 1,
		frame.mAltRects.begin(),
		frame.mAltRects.end());

	mFrameTimes.insert(
		mFrameTimes.begin() + frameIndex,
		frame.mTime);

	return true;
}

bool AnimationData::AddFrames(const std::vector<Animation::Frame>& frames)
{
	assert(false);
	return false;
}

bool AnimationData::RemoveFrame(const int index) {
	if (!FrameIndexIsValid(*this, index)) return false;

	using namespace std;

	mFrameTimes.erase(begin(mFrameTimes) + index);

	const auto eraseBegin = cbegin(mFrameRects) + (index * (mAltViewsPerFrame + 1));
	mFrameRects.erase(eraseBegin, eraseBegin + (mAltViewsPerFrame + 1));

	return true;
}

bool AnimationData::SetFrame(const int index, const Animation::Frame & frame)
{
	if (!FrameIndexIsValid(*this, index)) return false;
	if (!FrameIsValid(*this, frame)) return false;

	mFrameTimes[index] = frame.mTime;

	int rectIndex = GetRectIndex(*this, index, 0);

	mFrameRects[rectIndex] = frame.mBaseRect;

	for (const auto& rect : frame.mAltRects) {
		rectIndex++;
		mFrameRects[rectIndex + rectIndex] = rect;
	}

	return true;
}

bool TimeIsValid(const AnimationData::TimeUnit time) {
	return time > 0ms;

}

bool AnimationData::SetFrameTime(const int index, const TimeUnit time)
{
	if (!FrameIndexIsValid(*this, index)) return false;
	if (!TimeIsValid(time)) return false;

	mFrameTimes[index] = time;

	return true;
}

bool AnimationData::SetFrameRect(const int frameIndex, const int viewIndex, const Rect& rect)
{
	if (!FrameIndexIsValid(*this, frameIndex)) return false;
	if (!ViewIndexIsValid(*this, viewIndex)) return false;

	const int rectIndex = GetRectIndex(*this, frameIndex, viewIndex);

	mFrameRects[rectIndex] = rect;

	return true;
}

bool AnimationData::SwapFrames(const int index1, const int index2)
{
	if (!FrameIndexIsValid(*this, index1)) return false;
	if (!FrameIndexIsValid(*this, index2)) return false;

	using namespace std;

	swap(mFrameTimes[index1], mFrameTimes[index2]);

	const int rectsPerFrame = (mAltViewsPerFrame + 1);
	const auto srcBegin = begin(mFrameRects) + (index1 * rectsPerFrame);
	const auto srcEnd = srcBegin + rectsPerFrame;
	const auto dest = begin(mFrameRects) + (index2 * rectsPerFrame);
	swap_ranges(srcBegin, srcEnd, dest);

	return true;
}

}