#pragma once

#include <Box2D/Common/b2Math.h>

namespace qvr {

class Camera2D {
public:
	b2Transform mTransform;
	float mPixelsPerMetre = 32.0f;
	float mOffsetX;
	float mOffsetY;

	Camera2D()
	{
		mTransform.SetIdentity();
	}

	Camera2D(const b2Vec2& position, const float radians, const float width, const float height)
		: mTransform(position, b2Rot(radians)), mOffsetX(width), mOffsetY(height)
	{

	}

	Camera2D(const b2Vec2& position, const float radians, const float width, const float height, const float pixelsPerMetre)
		: mTransform(position, b2Rot(radians)), mOffsetX(width), mOffsetY(height), mPixelsPerMetre(pixelsPerMetre)
	{

	}

	void SetPosition(const b2Vec2 & position) {
		mTransform.p = position;
	}

	b2Vec2 GetPosition() const {
		return mTransform.p;
	}

	void MoveBy(const b2Vec2 & displacement) {
		mTransform.p += displacement;
	}

	void SetRotation(const float radians) {
		mTransform.q.Set(radians);
	}

	float GetRotation() const {
		return mTransform.q.GetAngle();
	}

	void RotateBy(const float radians) {
		mTransform.q.Set(mTransform.q.GetAngle() + radians);
	}

	inline float MetresToPixels(const float metres) const {
		return metres * mPixelsPerMetre;
	}

	inline float PixelsToMetres(const float pixels) const {
		return pixels / mPixelsPerMetre;
	}

	inline b2Vec2 WorldToCamera(const b2Vec2 & worldPos) const {
		b2Vec2 out = b2MulT(mTransform, worldPos);
		// Scale and add the offset (in pixels) from the top left of the screen
		// to the focal point of the camera (or is it the other way around?).
		out.x = MetresToPixels(out.x) + mOffsetX;
		out.y = MetresToPixels(out.y) + mOffsetY;
		return out;
	}

	inline b2Vec2 ScreenToWorld(const b2Vec2 & screenPos) const {
		return b2Mul(mTransform, b2Vec2(PixelsToMetres(screenPos.x - mOffsetX),
			PixelsToMetres(screenPos.y - mOffsetY)));
	}
};

void FreeControl(Camera2D& camera, float const dt);

auto GetFreeControlCamera2DInstructions() -> const char*;

}