#pragma once

#include <Box2D/Common/b2Math.h>
#include <optional.hpp>

class b2Fixture;
class b2World;
struct b2AABB;

namespace qvr {
class Entity;
class CustomComponent;
}

template <typename T>
using optional = std::experimental::optional<T>;

namespace FixtureFilterCategories
{
const uint16 Default = 1 << 0;
const uint16 Player = 1 << 1;
const uint16 Sensor = 1 << 2;
const uint16 Projectile = 1 << 14;
const uint16 RenderOnly = 1 << 15;
}

qvr::CustomComponent* GetCustomComponent(const b2Fixture* fixture);

qvr::Entity*          GetPlayerFromFixture(const b2Fixture* fixture);

std::experimental::optional<b2Vec2> QueryAABBToFindPlayer(
	const b2World& world,
	const b2AABB& aabb);

std::experimental::optional<b2Vec2> RayCastToFindPlayer(
	const b2World& world,
	const b2Vec2& rayStart,
	const b2Vec2& rayEnd);

std::experimental::optional<b2Vec2> RayCastToFindPlayer(
	const b2World& world,
	const b2Vec2& rayPos,
	const float angle,
	const float range);