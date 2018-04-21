#pragma once

#include <chrono>

namespace qvr
{
class RenderComponent;
}

inline float ApplyGravity(
	const float upVelocity, 
	const std::chrono::duration<float> deltaTime) 
{
	const float gravity = 10.0f;

	return upVelocity - (gravity * deltaTime.count());
}

inline float GetPositionDelta(
	const float velocity,
	const std::chrono::duration<float> deltaTime)
{
	return velocity * deltaTime.count();
}

void UpdateGroundOffset(
	qvr::RenderComponent& graphicsComponent,
	const float positionDelta);