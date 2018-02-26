#include "Utils.h"

#include <chrono>

#include <Box2D/Dynamics/b2Fixture.h>
#include <Box2D/Dynamics/b2WorldCallbacks.h>
#include <Box2D/Dynamics/b2World.h>

#include <Quiver/Entity/Entity.h>
#include <Quiver/Entity/CustomComponent/CustomComponent.h>
#include <Quiver/Entity/PhysicsComponent/PhysicsComponent.h>

using json = nlohmann::json;

using namespace std::chrono;

qvr::CustomComponent* GetCustomComponent(const b2Fixture* fixture)
{
	// Not every Fixture is guaranteed to have a RenderComponent,
	// but every Body is guaranteed to have a PhysicsComponent.
	const auto physicsComp = (qvr::PhysicsComponent*)fixture->GetBody()->GetUserData();

	assert(physicsComp);

	return physicsComp->GetEntity().GetCustomComponent();
}

qvr::Entity* GetPlayerFromFixture(const b2Fixture* fixture)
{
	if (const qvr::CustomComponent* inputComp = GetCustomComponent(fixture))
	{
		if (inputComp->GetTypeName() == "Player")
		{
			return &inputComp->GetEntity();
		}
	}

	return nullptr;
}

optional<b2Vec2> QueryAABBToFindPlayer(
	const b2World& world,
	const b2AABB& aabb)
{
	class FindPlayerQueryAABB : public b2QueryCallback
	{
	public:
		optional<b2Vec2> playerPosition;

		bool ReportFixture(b2Fixture* fixture) override
		{
			// Ignore members of the detached-RenderComponent category (0xF000).
			if ((fixture->GetFilterData().categoryBits & 0xF000) == 0xF000)
			{
				return true;
			}

			if (GetPlayerFromFixture(fixture))
			{
				playerPosition = fixture->GetBody()->GetPosition();
				return false;
			}

			return true;
		}
	};

	FindPlayerQueryAABB cb;

	world.QueryAABB(&cb, aabb);

	return cb.playerPosition;
}

optional<b2Vec2> RayCastToFindPlayer(
	const b2World& world,
	const b2Vec2& rayStart,
	const b2Vec2& rayEnd)
{
	class GetClosestFixtureCallback : public b2RayCastCallback
	{
	public:
		b2Fixture * closestFixture = nullptr;
		b2Vec2 closestPoint;

		float32 ReportFixture(
			b2Fixture* fixture,
			const b2Vec2& point,
			const b2Vec2& normal,
			float32 fraction)
			override
		{
			using namespace FixtureFilterCategories;
			const unsigned ignoreMask = RenderOnly | Projectile;

			if ((fixture->GetFilterData().categoryBits & ignoreMask) != 0)
			{
				return -1;
			}

			closestFixture = fixture;
			closestPoint = point;

			return fraction;
		}
	};

	GetClosestFixtureCallback cb;

	world.RayCast(&cb, rayStart, rayEnd);

	if (cb.closestFixture && GetPlayerFromFixture(cb.closestFixture))
	{
		return cb.closestPoint;
	}

	return {};
}

optional<b2Vec2> RayCastToFindPlayer(
	const b2World& world,
	const b2Vec2& rayPos,
	const float angle,
	const float range)
{
	const b2Vec2 rayDir = b2Mul(b2Rot(angle), b2Vec2(1.0f, 0.0f));
	const b2Vec2 rayEnd = rayPos + (range * rayDir);

	return RayCastToFindPlayer(world, rayPos, rayEnd);
}

void DeltaRadians::Update(const float currentRadians) {
	const float pi = b2_pi;
	const float tau = 2.0f * pi;

	float val = currentRadians - last;
	last = currentRadians;

	// Account for when the current and last are on different sides of the pole.
	val += (val > pi) ? -tau : (val < -pi) ? tau : 0.0f;

	// Limit the angle.
	const float max = pi * 0.25f;
	delta = std::max(-max, std::min(val, max));
}
