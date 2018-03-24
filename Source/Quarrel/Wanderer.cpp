#include "Wanderer.h"

#include <Box2D/Collision/Shapes/b2CircleShape.h>
#include <Box2D/Dynamics/b2Body.h>
#include <Box2D/Dynamics/b2Fixture.h>
#include <Box2D/Dynamics/b2World.h>
#include <Box2D/Dynamics/Contacts/b2Contact.h>
#include <ImGui/imgui.h>
#include <spdlog/spdlog.h>

#include "Quiver/Animation/Animators.h"
#include "Quiver/Entity/Entity.h"
#include "Quiver/Entity/PhysicsComponent/PhysicsComponent.h"
#include "Quiver/Entity/RenderComponent/RenderComponent.h"
#include "Quiver/World/World.h"

#include "Utils.h"

using namespace qvr;

static b2CircleShape CreateCircleShape(const float radius)
{
	b2CircleShape circle;
	circle.m_radius = radius;
	return circle;
}

Wanderer::Wanderer(Entity& entity) 
	: CustomComponent(entity) 
{
	m_WalkDirection = b2Vec2(-1.0f, 0.0f);

	b2FixtureDef fixtureDef;
	b2CircleShape circleShape = CreateCircleShape(5.0f);
	fixtureDef.shape = &circleShape;
	fixtureDef.isSensor = true;
	fixtureDef.filter.categoryBits = FixtureFilterCategories::Sensor;
	// Only collide with Players.
	fixtureDef.filter.maskBits     = FixtureFilterCategories::Player;
	m_Sensor = GetEntity().GetPhysics()->GetBody().CreateFixture(&fixtureDef);
}

Wanderer::~Wanderer() {}

bool IsPathClear(const b2World& world, b2Vec2 start, b2Vec2 direction, float distance) {
	class RayCastCallback : public b2RayCastCallback {
	public:
		inline float32 ReportFixture(b2Fixture* fixture, const b2Vec2& point,
			const b2Vec2& normal, float32 fraction)
			override
		{
			// Ignore members of the RenderOnly category.
			hitSomething = 
				(fixture->GetFilterData().categoryBits & FixtureFilterCategories::RenderOnly) == 0;
			return 0;
		}

		bool hitSomething = false;
	};

	RayCastCallback cb;

	world.RayCast(&cb, start, start + direction);

	return !cb.hitSomething;
}

void Wanderer::OnStep(const std::chrono::duration<float> timestep)
{
	{
		const Entity* collidingEntity =
			GetEntity().GetWorld().GetEntity(m_PlayerInSensor);
		if (collidingEntity) {
			// Run away!
			m_WalkDirection = 
				GetEntity().GetPhysics()->GetPosition() - 
				collidingEntity->GetPhysics()->GetPosition();
			m_WalkDirection.Normalize();

			GetEntity().GetGraphics()->SetObjectAngle(
				b2Atan2(m_WalkDirection.y, m_WalkDirection.x) + b2_pi);
		}
		else {
			m_PlayerInSensor = EntityId(0);
		}
	}

	// Walk in direction until a certain distance travelled is exceeded or
	// an object gets in the way (do a raycast to find out).
	const b2World& world = *GetEntity().GetWorld().GetPhysicsWorld();
	const b2Vec2 currentPos = GetEntity().GetPhysics()->GetPosition();

	if (IsPathClear(world, currentPos, m_WalkDirection, 1.0f)) {
		b2Body& body = GetEntity().GetPhysics()->GetBody();
		float walkForce = 2.0f;
		body.ApplyForce(walkForce * m_WalkDirection,
			b2Vec2(0.0f, 0.0f),
			true);
	} else {
		// Just flip direction.
		m_WalkDirection *= -1.0f;

		GetEntity().GetGraphics()->SetObjectAngle(
			b2Atan2(m_WalkDirection.y, m_WalkDirection.x) + b2_pi);
	}
}

void Wanderer::OnBeginContact(Entity& other, b2Fixture& myFixture, b2Fixture& otherFixture)
{
	auto log = spdlog::get("console");
	assert(log);

	if (other.GetCustomComponent()) {
		log->debug("Wanderer beginning contact with {}...", other.GetCustomComponent()->GetTypeName());
	}

	if (&myFixture == m_Sensor) {
		// We don't need to check if it's the Player because the sensor fixture is 
		// set up to only collide with the Player.
		m_PlayerInSensor = other.GetId();
	}
}

void Wanderer::OnEndContact(Entity& other, b2Fixture& myFixture, b2Fixture& otherFixture)
{
	auto log = spdlog::get("console");
	assert(log);

	if (other.GetCustomComponent()) {
		log->debug("Wanderer finishing contact with {}...", other.GetCustomComponent()->GetTypeName());
	}

	if (&myFixture == m_Sensor) {
		// We don't need to check if it's the Player because the sensor fixture is 
		// set up to only collide with the Player.
		m_PlayerInSensor = EntityId(0);
	}
}