#include "AnimationEditor.h"
#include <chrono>
#include <fstream>
#include <ImGui/imgui.h>
#include <ImGui/imgui-SFML.h>
#include <spdlog/fmt/fmt.h>

#include "Quiver/Misc/Logging.h"

namespace qvr {

using namespace std::literals::chrono_literals;
using namespace Animation;

// Returns true if every frame in the AnimationData is of the same size.
bool IsAnimationTiled(const AnimationData & animation)
{
	// If all the frame rects have the same width and height, then tiled is true.

	if (animation.GetRectCount() < 2) return false;

	Rect::Dimensions dimensionsOfFirstRect = GetDimensions(animation.GetRect(0, 0).value());

	for (int frameIndex = 0; frameIndex < animation.GetFrameCount(); frameIndex++) {
		for (int viewIndex = 1; viewIndex < animation.GetAltViewsPerFrame(); viewIndex++) {
			const auto dimensions = GetDimensions(
				animation.GetRect(frameIndex, viewIndex).value());
			if (dimensions != dimensionsOfFirstRect)
				return false;
		}
	}

	return true;
}

bool IsFlippedHorizontally(const Rect & frameRect)
{
	return frameRect.right < frameRect.left;
}

bool IsFlippedVertically(const Rect & frameRect)
{
	return frameRect.bottom < frameRect.top;
}

void FlipHorizontally(Rect& rect) {
	auto temp = rect.left;
	rect.left = rect.right;
	rect.right = temp;
}

void FlipVertically(Rect& rect) {
	auto temp = rect.top;
	rect.top = rect.bottom;
	rect.bottom = temp;
}

struct TileXY {
	int x, y;
};

// If an animation is tiled, use this to learn the tile coordinates of a frame in the animation.
TileXY DetermineTileXY(const Rect& frameRect,
	const Rect::Dimensions& tileDimensions)
{
	bool flippedHorizontally = IsFlippedHorizontally(frameRect);
	bool flippedVertically = IsFlippedVertically(frameRect);

	int actualLeft = flippedHorizontally ? frameRect.right : frameRect.left;
	int actualTop = flippedVertically ? frameRect.bottom : frameRect.top;

	return TileXY{
		actualLeft / tileDimensions.width,
		actualTop / tileDimensions.height
	};
}

void SetTileXY(Rect& frameRect,
	const Rect::Dimensions& tileDimensions,
	const TileXY& tileCoords)
{
	const auto tileXY = DetermineTileXY(frameRect, tileDimensions);

	int diffX = tileCoords.x - tileXY.x;
	int diffY = tileCoords.y - tileXY.y;

	// Convert from tile coords to texture coords.
	int textureDiffX = diffX * tileDimensions.width;
	int textureDiffY = diffY * tileDimensions.height;

	frameRect.right += textureDiffX;
	frameRect.left += textureDiffX;
	frameRect.top += textureDiffY;
	frameRect.bottom += textureDiffY;
}

int LastFrameIndex(const AnimationData& animation) {
	return std::max(0, animation.GetFrameCount() - 1);
}

int LastViewIndex(const AnimationData& animation) {
	return std::max(0, animation.GetAltViewsPerFrame() - 1);
}

bool AddCopyOfFrame(AnimationData& animation, const int frameIndex) {
	return animation.AddFrame(animation.GetFrame(frameIndex).value());
}

Rect EditRect(Animation::Rect rect) {
	{
		bool flippedHorizontally = IsFlippedHorizontally(rect);
		if (ImGui::Checkbox("Flip X", &flippedHorizontally)) {
			FlipHorizontally(rect);
		}
	}
	{
		bool flippedVertically = IsFlippedVertically(rect);
		if (ImGui::Checkbox("Flip Y", &flippedVertically)) {
			FlipVertically(rect);
		}
	}
	{
		int corner[2] = { rect.left, rect.top };
		if (ImGui::InputInt2("Top Left (X, Y)", corner)) {
			rect.left = corner[0];
			rect.top = corner[1];
		}
	}
	{
		int corner[2] = { rect.right, rect.bottom };
		if (ImGui::InputInt2("Bottom Right (X, Y)", corner)) {
			rect.right = corner[0];
			rect.bottom = corner[1];
		}
	}
	return rect;
}

void AnimationEditor::ProcessGui() {
	auto log = GetConsoleLogger();

	ImGui::Begin("Animation Editor", &mIsOpen);

	if (ImGui::CollapsingHeader("Texture")) {
		if (ImGui::InputText("Texture Filename", mTextureFilenameBuffer, 128,
			ImGuiInputTextFlags_EnterReturnsTrue))
		{
			mTexture.loadFromFile(mTextureFilenameBuffer);
		}

		ImGui::Image(mTexture);
	}

	if (ImGui::CollapsingHeader("Collection")
		&& !mAnimationCollection.empty())
	{
		int currentSelection = -1;

		auto itemsGetter = [](void* data, int idx, const char** out_text)
		{
			const auto& container = *(AnimationCollection*)(data);

			*out_text = container[idx].first.c_str();

			return true;
		};

		if (ImGui::ListBox("Select Animation", &currentSelection, itemsGetter,
			&mAnimationCollection, mAnimationCollection.size()))
		{
			mCurrentAnimName = mAnimationCollection[currentSelection].first;
			mCurrentAnim = mAnimationCollection[currentSelection].second;

			if (IsAnimationTiled(mCurrentAnim)) {
				mTiled = true;
				const Rect rect = mCurrentAnim.GetRect(0).value();
				mTileDimensions = GetDimensions(rect);
			}
			else {
				mTiled = false;
				mTileDimensions = { 1, 1 };
			}
		}

		ImGui::Indent();

		if (ImGui::CollapsingHeader("Save Collection")) {
			ImGui::InputText("JSON Filename##1", mAnimationCollectionFilename, 128);

			if (ImGui::Button("Save")) {
				if (strlen(mAnimationCollectionFilename) > 0)
				{
					nlohmann::json j;
					for (const auto& anim : mAnimationCollection)
					{
						const auto& name = anim.first;
						const auto& val = anim.second;
						j[name] = val.ToJson();
					}

					std::ofstream file(mAnimationCollectionFilename);
					if (file.is_open()) {
						file << j.dump(4);

						log->info(
							"AnimationEditor: Saved animation collection of size {} to {}",
							mAnimationCollection.size(),
							mAnimationCollectionFilename);
					}
				}
			}
		}

		ImGui::Unindent();
	}

	if (ImGui::CollapsingHeader("Load File")) {
		ImGui::InputText("JSON Filename to Load", mAnimationToLoadFilename, 128);
		if (strlen(mAnimationToLoadFilename) > 0) {
			if (ImGui::Button("Load Individual")) {
				std::ifstream file(mAnimationToLoadFilename);
				if (file.is_open()) {
					nlohmann::json j;
					j << file;
					mCurrentAnim.Clear();
					if (const auto newAnim = AnimationData::FromJson(j)) {
						// Success!
						mCurrentAnimName = mAnimationToLoadFilename;

						mCurrentAnim = *newAnim;

						if (IsAnimationTiled(mCurrentAnim)) {
							mTiled = true;
							const Rect rect = mCurrentAnim.GetRect(0).value();
							mTileDimensions = GetDimensions(rect);
						}
						else {
							mTiled = false;
							mTileDimensions = { 1, 1 };
						}
					}
				}
				else {
					log->error("AnimationEditor: Failed to load '{}'", mAnimationToLoadFilename);
				}
			}

			ImGui::SameLine();

			if (ImGui::Button("Load Collection")) {
				strcpy(mAnimationCollectionFilename, mAnimationToLoadFilename);

				if (strlen(mAnimationCollectionFilename) > 0) {
					std::ifstream file(mAnimationCollectionFilename);
					if (file.is_open()) {
						nlohmann::json j;
						j << file;

						mAnimationCollection.clear();

						// Read into map.
						auto animJsons = j.get<std::unordered_map<std::string, nlohmann::json>>();

						int count = 0;

						// Create collection.
						for (auto& kvp : animJsons) {
							auto name = kvp.first;
							if (const auto anim = AnimationData::FromJson(kvp.second)) {
								mAnimationCollection.push_back(std::make_pair(name, *anim));
								count++;
							}
							else {
								log->error("AnimationEditor: Failed to load '{}'", name);
							}
						}

						log->info(
							"AnimationEditor: Loaded {} animations into collection from '{}'",
							count, 
							mAnimationCollectionFilename);
					}
				}
			}
		}
	}

	auto PutFrame = [](const sf::Texture& texture, const Rect& frameRect) {
		ImGui::Image(texture,
			sf::FloatRect((float)frameRect.left,
			(float)frameRect.top,
				(float)(frameRect.right - frameRect.left),
				(float)(frameRect.bottom - frameRect.top)));
	};

	const Animation::Frame defaultFrame{ 1ms, Rect{}, {} };

	if (ImGui::CollapsingHeader("Current Animation")) {
		if (ImGui::Button("Clear"))
		{
			mCurrentAnim.Clear();
			mCurrentAnimName.clear();
			mCurrentFrameIndex = 0;
			mCurrentViewIndex = 0;
		}

		if (ImGui::CollapsingHeader("Add Frame")) {
			if (ImGui::Button("Add Frame##btn"))
			{
				mCurrentAnim.AddFrame(defaultFrame);
				mCurrentFrameIndex = LastFrameIndex(mCurrentAnim);
			}

			if (mCurrentAnim.GetFrameCount() > 0)
			{
				if (ImGui::Button("Add Copy of Current Frame"))
				{
					AddCopyOfFrame(mCurrentAnim, mCurrentFrameIndex);
					mCurrentFrameIndex = LastFrameIndex(mCurrentAnim);
				}
			}
		}

		if (ImGui::CollapsingHeader("Insert Frame")) {
			if (ImGui::Button("Insert Frame##btn")) {
				mCurrentAnim.InsertFrame(
					defaultFrame,
					mCurrentFrameIndex);
			}

			if (mCurrentAnim.GetFrameCount() > 0)
			{
				if (ImGui::Button("Insert Copy of Current Frame")) {
					mCurrentAnim.InsertFrame(
						mCurrentAnim.GetFrame(mCurrentFrameIndex).value(),
						mCurrentFrameIndex);
				}
			}
		}

		if (mCurrentAnim.GetFrameCount() > 0)
		{
			if (ImGui::CollapsingHeader("Edit Frame"))
			{
				if (ImGui::Button("Remove Frame"))
				{
					mCurrentAnim.RemoveFrame(mCurrentFrameIndex);
					mCurrentFrameIndex = std::max(0, mCurrentFrameIndex - 1);
				}

				ImGui::Text("Frame Count: %d", mCurrentAnim.GetFrameCount());
				ImGui::Text("Rect Count:  %d", mCurrentAnim.GetRectCount());
				ImGui::Text("Views Per Frame: %d", mCurrentAnim.GetAltViewsPerFrame() + 1);

				ImGui::SliderInt("Frame Index", (int*)&mCurrentFrameIndex, 0, LastFrameIndex(mCurrentAnim));
				ImGui::SliderInt("View Index", (int*)&mCurrentViewIndex, 0, LastViewIndex(mCurrentAnim));

				const bool canMoveLeft = mCurrentFrameIndex > 0;
				const bool canMoveRight = mCurrentFrameIndex < LastFrameIndex(mCurrentAnim);

				if (canMoveLeft && ImGui::Button("Move Left")) {
					mCurrentAnim.SwapFrames(mCurrentFrameIndex, mCurrentFrameIndex - 1);
					mCurrentFrameIndex--;
				}

				// Do this so that the "Move Right" button appears next to the "Move Left" button, rather
				// than below.
				if (canMoveLeft && canMoveRight) {
					ImGui::SameLine();
				}

				if (canMoveRight && ImGui::Button("Move Right"))
				{
					mCurrentAnim.SwapFrames(mCurrentFrameIndex, mCurrentFrameIndex + 1);
					mCurrentFrameIndex++;
				}

				{
					Rect rect = mCurrentAnim.GetRect(mCurrentFrameIndex, mCurrentViewIndex).value();

					PutFrame(mTexture, rect);

					rect = EditRect(rect);

					mCurrentAnim.SetFrameRect(mCurrentFrameIndex, mCurrentViewIndex, rect);
				}

				int time = mCurrentAnim.GetTime(mCurrentFrameIndex).value().count();
				if (ImGui::SliderInt("Time (Milliseconds)", &time, 1, 1000)) {
					mCurrentAnim.SetFrameTime(mCurrentFrameIndex, (AnimationData::TimeUnit)time);
				}
			}

			// Draw all the frames in the animation, with the current view, side by side:
			{
				// How far across the window we are.
				int w = 0;

				for (int frameIndex = 0; frameIndex < mCurrentAnim.GetFrameCount(); ++frameIndex) {
					const Rect frameRect = mCurrentAnim.GetRect(frameIndex, mCurrentViewIndex).value();

					// Pixels to leave blank between frames.
					const int padding = 10;

					// Keep going or drop down a line.
					if (w < ImGui::GetWindowWidth()) {
						ImGui::SameLine();
					}
					else {
						w = GetWidth(frameRect) + padding;
					}

					PutFrame(mTexture, frameRect);

					w += GetWidth(frameRect) + padding;
				}
			}
		}

		if (mCurrentAnim.IsValid()) {
			{
				char buffer[128] = {};
				if (!mCurrentAnimName.empty()) {
					strcpy(buffer, mCurrentAnimName.c_str());
				}
				if (ImGui::InputText("Animation Name", buffer, 128)) {
					mCurrentAnimName = buffer;
				}
			}

			if (ImGui::Button("Save To Current Collection")) {
				bool found = false;

				for (auto& entry : mAnimationCollection) {
					if (entry.first == mCurrentAnimName) {
						entry = std::make_pair(mCurrentAnimName, mCurrentAnim);
						log->info(
							"AnimationEditor: Overwrote AnimationData in collection under the name '{}'", 
							mCurrentAnimName);
						found = true;
						break;
					}
				}

				if (!found) {
					mAnimationCollection.push_back(make_pair(mCurrentAnimName, mCurrentAnim));
					log->info("AnimationEditor: Added AnimationData to collection under the name '{}'", mCurrentAnimName);
				}
			}

			if (ImGui::Button("Save to JSON File")) {
				if (mCurrentAnimName.size() > 0) {
					const nlohmann::json j = mCurrentAnim.ToJson();
					std::ofstream file(mCurrentAnimName);
					if (!file.is_open()) {
						log->error("AnimationEditor: File '{}' is already open", mCurrentAnimName);
					}
					else {
						file << j.dump(4);
						log->error("AnimationEditor: Saved current animation to '{}'", mCurrentAnimName);
					}
				}
				else {
					log->error("AnimationEditor: Cannot save to file without a name");
				}
			}

		}
		else {
			ImGui::Text("Animation is invalid - cannot save.");
		}
	}

	if (ImGui::CollapsingHeader("Preview")) {
		if (ImGui::Button("Update Preview")) {
			if (mCurrentAnimationId != AnimationId::Invalid && 
				mAnimators.GetAnimations().Contains(mCurrentAnimationId)) 
			{
				mAnimators.RemoveAnimation(mCurrentAnimationId);
			}
			
			mCurrentAnimationId = mAnimators.AddAnimation(mCurrentAnim);
			
			if (mCurrentAnimationId != AnimationId::Invalid) 
			{
				mAnimatorId = mAnimators.AddAnimator(mAnimationPreviewRect, mCurrentAnimationId);
			}
		}

		ImGui::SliderFloat("Playback Speed Multiplier", &mAnimationPlaybackSpeedMultiplier, 0.1f, 5.0f);

		if (ImGui::SliderAngle("View Angle", &mAnimationPreviewViewAngle, 0, 360.0f)) {
			mAnimators.UpdateAnimatorAltView(mAnimatorId, mAnimationPreviewObjectAngle, mAnimationPreviewViewAngle);
		}

		if (ImGui::SliderAngle("Object Angle", &mAnimationPreviewObjectAngle, 0, 360.0f)) {
			mAnimators.UpdateAnimatorAltView(mAnimatorId, mAnimationPreviewObjectAngle, mAnimationPreviewViewAngle);
		}

		PutFrame(mTexture, mAnimationPreviewRect.rect);
	}

	ImGui::End();
}

void AnimationEditor::Update(const float dt) {
	if (!IsOpen()) {
		return;
	}

	mTimeCounter += dt * mAnimationPlaybackSpeedMultiplier;

	const auto timestep = 1.0f / 60.0f;

	if (mTimeCounter >= timestep) {
		mAnimators.Animate(AnimatorCollection::TimeUnit(int(timestep * 1000)));
		mTimeCounter = 0.0f;
	}
}

}