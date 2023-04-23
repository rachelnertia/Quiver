#pragma once

#include <chrono>

#include <Box2D/Common/b2Math.h>

#include <json.hpp>

#include "Quiver/Entity/Component.h"
#include "Quiver/Physics/PhysicsUtils.h"

class b2Body;
class b2Fixture;
class b2Shape;
struct b2BodyDef;
struct b2FixtureDef;
struct b2Transform;

namespace qvr {

class World;
struct PhysicsComponentDef;

class PhysicsComponent final : public Component {
public:
	explicit PhysicsComponent(Entity&entity, const PhysicsComponentDef& def);

	~PhysicsComponent();

	PhysicsComponent(const PhysicsComponent&) = delete;
	PhysicsComponent(const PhysicsComponent&&) = delete;

	PhysicsComponent& operator=(const PhysicsComponent&) = delete;
	PhysicsComponent& operator=(const PhysicsComponent&&) = delete;

	nlohmann::json ToJson(); // Maybe make this ToPhysicsComponentDef?

	b2Vec2 GetPosition() const;

	b2Body& GetBody() { return *mBody; }

	float GetBottom() const { return mGroundOffset; }
	float GetTop() const { return mGroundOffset + mHeight; }

	void SetBottom(float bottom) { mGroundOffset = bottom; }
	void SetTop(float top) { mGroundOffset = top - mHeight; }

	float GetNextBottom() const { return CalculateNextGroundOffset(); }
	float GetNextTop() const { return CalculateNextGroundOffset() + mHeight; }

	bool IsGrounded() const { return mGroundFixture != nullptr || mGroundOffset <= 0.0f; }

	void SetGrounded(b2Fixture* groundFixture = nullptr);
	void SetNotGrounded();

	void SetZVelocity(float velocity);

	void Update(const std::chrono::duration<float> deltaTime);

private:
	float CalculateNextZVelocity() const;
	float CalculateNextGroundOffset() const;

	Physics::b2BodyUniquePtr mBody;

	float mGroundOffset = 0.0f;
	float mHeight = 1.0f;
	float mZVelocity = 0.0f;

	// Pointer to the fixture this object is on top of.
	// Probably make this a EntityID so it's not potentially a dangling pointer.
	b2Fixture* mGroundFixture = nullptr;
};

}