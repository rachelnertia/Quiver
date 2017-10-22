#pragma once

#include <Box2D/Dynamics/b2Body.h>
#include <Box2D/Dynamics/b2Fixture.h>

#include <json.hpp>

namespace qvr {

struct PhysicsComponentDef
{
	std::unique_ptr<b2Shape> m_Shape;
	b2FixtureDef fixtureDef;
	b2BodyDef bodyDef;

	static bool VerifyJson(const nlohmann::json & j);

	PhysicsComponentDef(const b2Shape& shape, const b2Vec2& position, const float angle);
	PhysicsComponentDef(const nlohmann::json& j);
};

}