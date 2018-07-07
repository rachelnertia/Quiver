#pragma once

#include <Box2D/Common/b2Math.h>
#include <json.hpp>

namespace sf {
class RenderTarget;
class Window;
}

namespace qvr {

class World;

class Camera3D {
public:

	Camera3D() = default;

	Camera3D(const b2Transform transform)
		: mTransform(transform)
	{}

	Camera3D(const Camera3D&);
	Camera3D(const Camera3D&&) = delete;

	Camera3D& operator=(const Camera3D&);
	Camera3D& operator=(const Camera3D&&) = delete;

	bool ToJson(nlohmann::json& j, const World* world) const;
	bool FromJson(const nlohmann::json& j, World* world);

	using OverlayDrawer = std::function<void(sf::RenderTarget&)>;

	void SetOverlayDrawer(OverlayDrawer overlayDrawer) {
		mOverlayDrawer = overlayDrawer;
	}

	void DrawOverlay(sf::RenderTarget& target) const {
		if (mOverlayDrawer) {
			mOverlayDrawer(target);
		}
	}

	const b2Vec2& GetPosition() const { return mTransform.p; }
	const b2Vec2 GetForwards() const { return mTransform.q.GetYAxis(); }
	const b2Vec2 GetRightwards() const { return mTransform.q.GetXAxis(); }

	float GetRotation() const { return mTransform.q.GetAngle(); }
	float GetHeight() const { return mHeight; }
	float GetBaseHeight() const { return mBaseHeight; }
	float GetHeightOffset() const { return mBaseHeight - mHeight; }
	float GetPitchRadians() const { return mPitchRadians; }
	float GetFovRadians() const { return mFovRadians; }
	float GetViewPlaneWidthModifier() const { return mFovRadians / b2_pi; } // TODO: This needs a better name.

	void SetPosition(const b2Vec2& position) { mTransform.p = position; }
	void SetRotation(const float radians) { mTransform.q.Set(radians); }
	void SetHeight(const float height) {
		mHeight = std::max(height, 0.0f);
	}
	void SetFov(const float radians) {
		const float Maximum = b2_pi * 0.75f;
		const float Minimum = b2_pi * 0.25f;
		mFovRadians = std::min(std::max(Minimum, radians), Maximum);
	}
	void SetPitch(const float radians)
	{
		const float Maximum = b2_pi / 4;
		const float Minimum = -Maximum;
		mPitchRadians = std::min(std::max(Minimum, radians), Maximum);
	}

	void RotateBy(const float horizontalRadians) {
		mTransform.q.Set(GetRotation() + horizontalRadians);
	}
	void MoveBy(const b2Vec2 & displacement) {
		mTransform.p += displacement;
	}

private:
	b2Transform mTransform = b2Transform(b2Vec2_zero, b2Rot(0.0f));

	float mHeight = 0.5f;
	float mBaseHeight = 0.5f;
	float mPitchRadians = 0.0f;
	float mFovRadians = b2_pi / 2;

	OverlayDrawer mOverlayDrawer;

};

void FreeControl(
	Camera3D& camera,
	const float dt,
	const bool mouselook = false,
	const sf::Window* windowForMouselook = nullptr);

auto GetFreeControlCamera3DInstructions() -> const char*;

inline int GetPitchOffsetInPixels(const Camera3D& camera, const int targetHeightinPixels) {
	return (int)(tan(camera.GetPitchRadians()) * targetHeightinPixels);
}

}