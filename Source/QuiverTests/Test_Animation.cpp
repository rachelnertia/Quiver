#include <Catch.hpp>

#include "Quiver/Animation/AnimationData.h"
#include "Quiver/Animation/Animators.h"
#include "Quiver/Misc/Logging.h"

using namespace std::chrono_literals;
using json = nlohmann::json;

using namespace qvr;
using namespace qvr::Animation;

TEST_CASE("AnimationFrame", "[Animation]")
{
	Rect rect{ 0, 0, 1, 1 };
	const auto time = 1ms;
	Frame frame{ time, rect, {} };

	REQUIRE(frame.mBaseRect == rect);
	REQUIRE(frame.mAltRects.empty());
	REQUIRE(frame.mTime == time);
}

TEST_CASE("AnimationData can be converted from json and back")
{
	// FromJson fails when json is invalid.
	REQUIRE(!AnimationData::FromJson(json()).has_value());
	// ToJson fails when AnimationData is invalid.
	REQUIRE(AnimationData().ToJson().empty());

	const json inputJson = {
		{ "frameRects", {
			{{"top",0},{"bottom",1},{"left",0},{"right",1}},
			{{"top",0},{"bottom",1},{"left",1},{"right",2}}
		}},
		{ "frameTimes", { 1, 2 } }
	};

	const auto animationMaybe = AnimationData::FromJson(inputJson);
	REQUIRE(animationMaybe.has_value());
	const AnimationData animation = animationMaybe.value();
	REQUIRE(animation.IsValid());
	REQUIRE(animation.GetFrameCount() == 2);
	REQUIRE(animation.GetRectCount() == 2);
	REQUIRE(animation.GetAltViewsPerFrame() == 0);

	auto RectFromJson = [](const json j) {
		Rect rect;
		rect.FromJson(j);
		return rect;
	};

	for (int frameIndex = 0; frameIndex < animation.GetFrameCount(); frameIndex++)
	{
		// Check that frame time is correct.
		const Animation::TimeUnit ms = 
			Animation::TimeUnit(inputJson["frameTimes"][frameIndex].get<int>());
		REQUIRE(ms == animation.GetTime(frameIndex).value());

		// Check that frame rect is correct.
		const Rect rect = RectFromJson(inputJson["frameRects"][frameIndex]);
		REQUIRE(rect == animation.GetRect(frameIndex).value());
	}

	const json outputJson = animation.ToJson();

	REQUIRE(outputJson == inputJson);
}

TEST_CASE("Can use SetFrameRect to modify specific Rects in an AnimationData", "[Animation]")
{
	AnimationData animationData;

	REQUIRE(animationData.SetFrameRect(0, 0, Rect{}) == false);

	const Frame testFrames[2] =
	{
		Frame{ 1ms,{ 0,0,1,1 },{} },
		Frame{ 2ms,{ 1,1,0,0 },{} }
	};

	animationData.AddFrame(testFrames[0]);

	// Check that invalid indices are rejected.
	{
		const int fc = animationData.GetFrameCount();
		const int vc = animationData.GetAltViewsPerFrame() + 1;
		REQUIRE(animationData.SetFrameRect(-1, 0, Rect{}) == false);
		REQUIRE(animationData.SetFrameRect(fc, 0, Rect{}) == false);
		REQUIRE(animationData.SetFrameRect(0, -1, Rect{}) == false);
		REQUIRE(animationData.SetFrameRect(0, vc, Rect{}) == false);
	}
	// And that the frame rect is left unmodified.
	REQUIRE(animationData.GetRect(0, 0).value() == testFrames[0].mBaseRect);
	
	REQUIRE(animationData.SetFrameRect(0, 0, Rect{}));
	REQUIRE(animationData.GetRect(0, 0).value() == Rect{});
}

TEST_CASE("AnimationData can be modified", "[Animation]")
{
	AnimationData animationData;
	
	REQUIRE(animationData.GetFrameCount() == 0);
	REQUIRE(animationData.GetRectCount() == 0);

	REQUIRE(animationData.IsValid() == false);

	REQUIRE(animationData.ToJson().empty());

	SECTION("AddFrame rejects invalid frames") {
		Frame frame;
		frame.mTime = -1ms;
		REQUIRE(animationData.AddFrame(frame) == false);

		frame.mTime = 0ms;
		REQUIRE(animationData.AddFrame(frame) == false);

		frame.mTime = 1ms;
		frame.mAltRects.push_back(Rect{});
		REQUIRE(animationData.AddFrame(frame) == false);

		REQUIRE(animationData.GetFrameCount() == 0);
		REQUIRE(animationData.GetRectCount() == 0);
	}

	const Frame frame1{ 1ms, { 0,0,1,1 }, {} };

	REQUIRE(animationData.AddFrame(frame1));

	REQUIRE(animationData.IsValid() == false);
	REQUIRE(animationData.GetFrameCount() == 1);
	REQUIRE(animationData.GetFrame(0).value() == frame1);

	const Frame frame2{ 2ms, { 1,0,2,0 }, {} };
	
	REQUIRE(animationData.AddFrame(frame2));

	REQUIRE(animationData.IsValid());
	REQUIRE(animationData.GetFrameCount() == 2);
	
	{
		const Frame val = animationData.GetFrame(0).value();
		REQUIRE(val == frame1);
	}
	{
		const Frame val = animationData.GetFrame(1).value();
		REQUIRE(val == frame2);
	}

	SECTION("Can use SetFrame to modify existing frames") {
		SECTION("SetFrame rejects invalid indices") {
			REQUIRE(!animationData.SetFrame(-1, frame2));
			REQUIRE(!animationData.SetFrame(animationData.GetFrameCount(), frame2));
		}
		SECTION("SetFrame rejects invalid frames") {
			const Frame frame3{ 3ms,{ 9,9,9,9 },{ { 0,1,2,3 } } };
			// Number of alt views does not the AnimationData's.
			REQUIRE(!animationData.SetFrame(0, frame3));
		}
		const Frame frame3{ 3ms, {9,9,9,9}, {} };
		REQUIRE(animationData.SetFrame(0, frame3));
		REQUIRE(animationData.GetFrame(0).value() == frame3);
	}
	SECTION("Can use SwapFrames to swap two frames") {
		SECTION("SwapFrames rejects invalid indices") {
			REQUIRE(!animationData.SwapFrames(-1, 0));
			REQUIRE(!animationData.SwapFrames(0, -1));
			REQUIRE(!animationData.SwapFrames(-1, -1));
			REQUIRE(!animationData.SwapFrames(animationData.GetFrameCount(), 0));
			REQUIRE(!animationData.SwapFrames(0, animationData.GetFrameCount()));
			REQUIRE(!animationData.SwapFrames(animationData.GetFrameCount(), animationData.GetFrameCount()));
			// Nothing's been modified.
			REQUIRE(animationData.GetFrame(0).value() == frame1);
			REQUIRE(animationData.GetFrame(1).value() == frame2);
		}

		REQUIRE(animationData.SwapFrames(0, 1));

		REQUIRE(animationData.GetFrame(0).value() == frame2);
		REQUIRE(animationData.GetFrame(1).value() == frame1);
	}
	SECTION("Can use RemoveFrame to remove a frame") {
		SECTION("RemoveFrame rejects invalid indices") {
			REQUIRE(animationData.RemoveFrame(-1) == false);
			REQUIRE(animationData.RemoveFrame(animationData.GetFrameCount()) == false);
			REQUIRE(animationData.RemoveFrame(animationData.GetFrameCount() + 1) == false);
		}
		REQUIRE(animationData.RemoveFrame(0));
		REQUIRE(animationData.GetFrameCount() == 1);
		REQUIRE(animationData.GetRectCount() == 1);
	}
	SECTION("Can use Clear to reset everything") {
		animationData.Clear();

		REQUIRE(animationData.GetFrameCount() == 0);
		REQUIRE(animationData.GetRectCount() == 0);

		REQUIRE(animationData.IsValid() == false);
	}
}

TEST_CASE("AnimatorCollection", "[Animation]")
{
	qvr::InitLoggers(spdlog::level::off);

	AnimatorCollection animators;

	// Animate is fine when there's no Animations and no Animators
	animators.Animate(10ms);

	AnimationData animationData;

	{
		const AnimationId animationId = GenerateAnimationId(animationData);
		REQUIRE(animationId == AnimationId::Invalid);
		//REQUIRE(animators.AddAnimation(animationData) == AnimationId::Invalid);
	}

	animationData.AddFrame(Frame{ 10ms, Rect{ 0,0,1,1 },{} });
	animationData.AddFrame(Frame{ 10ms, Rect{ 1,0,2,1 },{} });

	const AnimationId animationId = GenerateAnimationId(animationData);
	REQUIRE(animationId != AnimationId::Invalid);

	{
		AnimatorTarget animatorTarget{};
		REQUIRE(animators.Add(animatorTarget, animationId) == AnimatorId::Invalid);
	}

	REQUIRE(animators.AddAnimation(animationData) == animationId);
	
	AnimatorTarget animatorTarget{};
	const AnimatorId animatorId = animators.Add(animatorTarget, animationId);

	REQUIRE(animatorId != AnimatorId::Invalid);
	REQUIRE(animators.GetAnimation(animatorId) == animationId);
	REQUIRE(animators.GetFrame(animatorId) == 0);
	REQUIRE(animatorTarget.views.views[0] == animationData.GetRect(0).value());

	SECTION("Animate plays the animation") {
		for (int i = 0; i < animationData.GetFrameCount(); ++i) {
			REQUIRE(animators.GetAnimation(animatorId) == animationId);
			REQUIRE((int)animators.GetFrame(animatorId) == i);
			REQUIRE(animatorTarget.views.views[0] == animationData.GetRect(i).value());

			animators.Animate(animationData.GetTime(i).value());
		}

		SECTION("It loops") {
			REQUIRE(animators.GetAnimation(animatorId) == animationId);
			REQUIRE(animators.GetFrame(animatorId) == 0);
			REQUIRE(animatorTarget.views.views[0] == animationData.GetRect(0).value());
		}
	}

	SECTION("RemoveAnimation removes the animation and the animator that references it") {
		REQUIRE(animators.RemoveAnimation(animationId));
		REQUIRE(animators.Exists(animatorId) == false);
	}

	SECTION("Remove removes the animator but keeps the animation") {
		REQUIRE(animators.Remove(animatorId));
		REQUIRE(animators.Exists(animatorId) == false);
	}

	SECTION("SetFrame") {
		SECTION("Reject invalid AnimatorIds") {
			for (const AnimatorId invalidAnimatorId : { AnimatorId::Invalid, AnimatorId(animatorId.GetValue() + 1) })
			{
				REQUIRE(animators.SetFrame(invalidAnimatorId, 0) == false);
			}
		}

		const Rect originalTargetVal = animatorTarget.views.views[0];
		const int originalFrame = animators.GetFrame(animatorId);

		SECTION("Reject invalid frame") {
			for (const int invalidFrame : { -1, animationData.GetFrameCount() })
			{
				REQUIRE(animators.SetFrame(animatorId, invalidFrame) == false);
				REQUIRE((int)animators.GetFrame(animatorId) == originalFrame);
				REQUIRE(animatorTarget.views.views[0] == originalTargetVal);
			}
		}

		SECTION("Set to current frame") {
			REQUIRE(animators.SetFrame(animatorId, 0));
			REQUIRE((int)animators.GetFrame(animatorId) == originalFrame);
			REQUIRE(animatorTarget.views.views[0] == originalTargetVal);
		}

		SECTION("Set to other frame") {
			const int otherIndex = 1;

			REQUIRE(animators.SetFrame(animatorId, otherIndex));
			REQUIRE(animators.GetFrame(animatorId) == otherIndex);
			REQUIRE(animatorTarget.views.views[0] == animationData.GetRect(otherIndex).value());
		}
	}

	SECTION("SetAnimation") {
		SECTION("SetAnimation fails if given an invalid AnimationId") {
			REQUIRE(animators.SetAnimation(animatorId, AnimationId::Invalid) == false);
		}

		SECTION("SetAnimation works with the current animation") {
			const int currentFrame = animators.GetFrame(animatorId);
			const Rect currentRect = animatorTarget.views.views[0];

			REQUIRE(animators.SetAnimation(animatorId, animationId));
			
			// Check that the animator & target are left unchanged
			REQUIRE(animators.GetAnimation(animatorId) == animationId);
			REQUIRE((int)animators.GetFrame(animatorId) == currentFrame);
			REQUIRE(currentRect == animatorTarget.views.views[0]);
		}

		SECTION("SetAnimation works with different animation") {
			{
				int frameIndex = 0;
				Animation::Frame frame = animationData.GetFrame(frameIndex).value();
				frame.mBaseRect = Rect{ 2, 0, 3, 1 };
				animationData.SetFrame(frameIndex, frame);
				
				frameIndex = 1;
				frame = animationData.GetFrame(frameIndex).value();
				frame.mBaseRect = Rect{ 3, 0, 4, 1 };
				animationData.SetFrame(frameIndex, frame);
			}

			const AnimationId newAnimationId = GenerateAnimationId(animationData);

			SECTION("... but not if it hasn't been added yet") {
				REQUIRE(
					animators.SetAnimation(
						animatorId, 
						AnimatorStartSetting(newAnimationId)) 
					== false);

				// TODO: Check that the animator is left unchanged
			}

			animators.AddAnimation(animationData);
			
			REQUIRE(
				animators.SetAnimation(
					animatorId, 
					AnimatorStartSetting(newAnimationId)));

			REQUIRE(
				animators.GetAnimation(animatorId) 
				== newAnimationId);

			// ...
		}
	}

	SECTION("QueueAnimation") {

	}
}

TEST_CASE("AnimationLibrary", "[Animation]") {
	qvr::InitLoggers(spdlog::level::off);

	AnimationLibrary animations;

	REQUIRE(animations.Contains(AnimationId::Invalid) == false);
	REQUIRE(animations.Contains(AnimationId(1)) == false);

	REQUIRE(animations.GetCount() == 0);
	REQUIRE(animations.GetIds().empty());

	{
		const json j = animations;
		REQUIRE(j.empty());
	}

	AnimationData animationData;

	REQUIRE(animations.Add(animationData) == AnimationId::Invalid);

	animationData.AddFrame(Frame{ 10ms, Rect{ 0,0,1,1 },{} });
	animationData.AddFrame(Frame{ 10ms, Rect{ 1,0,2,1 },{} });

	const AnimationId animationId = GenerateAnimationId(animationData);

	REQUIRE(animationId != AnimationId::Invalid);

	// Can't remove animation that hasn't been added.
	REQUIRE(animations.Remove(animationId) == false);
	REQUIRE(animations.Remove(AnimationId::Invalid) == false);

	animations.Add(animationData);

	REQUIRE(animations.GetCount() == 1);
	REQUIRE(animations.GetFrameCount(animationId) == animationData.GetFrameCount());
	REQUIRE(animations.HasAltViews(animationId) == false);
	REQUIRE(animations.Contains(animationId));

	// No source info.
	REQUIRE(animations.GetSourceInfo(animationId).has_value() == false);

	{
		const auto animationIds = animations.GetIds();
		REQUIRE(animationIds.size() == 1);
		REQUIRE(animationIds[0] == animationId);
	}

	animationData.AddFrame(Frame{ 10ms, Rect{ 1,0,2,1 },{} });

	const AnimationId animationId2 = animations.Add(animationData);

	REQUIRE(animations.GetCount() == 2);

	REQUIRE(animations.Remove(animationId));

	REQUIRE(animations.GetRect(animationId2, 0) == animationData.GetRect(0).value());
	REQUIRE(animations.GetRect(animationId2, 1) == animationData.GetRect(1).value());
	REQUIRE(animations.GetRect(animationId2, 2) == animationData.GetRect(2).value());
}