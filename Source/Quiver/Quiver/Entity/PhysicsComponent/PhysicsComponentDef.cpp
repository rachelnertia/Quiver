#include "PhysicsComponentDef.h"

#include <Box2D/Collision/Shapes/b2CircleShape.h>
#include <Box2D/Collision/Shapes/b2ChainShape.h>
#include <Box2D/Collision/Shapes/b2EdgeShape.h>
#include <Box2D/Collision/Shapes/b2PolygonShape.h>
#include <spdlog/spdlog.h>

#include "Quiver/Physics/PhysicsShape.h"

namespace qvr {

std::unique_ptr<b2Shape> DynamicCopy(const b2Shape& shape)
{
	switch (shape.GetType())
	{
	case b2Shape::Type::e_circle:
	{
		auto circleShape = std::make_unique<b2CircleShape>();
		*(circleShape.get()) = (b2CircleShape&)shape;
		return circleShape;
	}
	case b2Shape::Type::e_polygon:
	{
		auto polyShape = std::make_unique<b2PolygonShape>();
		*(polyShape.get()) = (b2PolygonShape&)shape;
		return polyShape;
	}
	case b2Shape::Type::e_chain:
	{
		auto chainShape = std::make_unique<b2ChainShape>();
		*(chainShape.get()) = (b2ChainShape&)shape;
		return chainShape;
	}
	case b2Shape::Type::e_edge:
	{
		auto edgeShape = std::make_unique<b2EdgeShape>();
		*(edgeShape.get()) = (b2EdgeShape&)shape;
		return edgeShape;
	}
	}

	return nullptr;
}

PhysicsComponentDef::PhysicsComponentDef(const b2Shape & shape, const b2Vec2 & position, const float angle)
	: m_Shape(DynamicCopy(shape))
{
	fixtureDef = b2FixtureDef{};
	bodyDef = b2BodyDef{};

	fixtureDef.density = 1.0f;
	fixtureDef.shape = m_Shape.get();

	bodyDef.position = position;
	bodyDef.type = b2_staticBody;
	bodyDef.angle = angle;
}

PhysicsComponentDef::PhysicsComponentDef(const nlohmann::json & j)
{
	fixtureDef = b2FixtureDef{};
	bodyDef = b2BodyDef{};

	using json = nlohmann::json;

	if (j.find("Shape") == j.end()) return;

	{
		m_Shape = PhysicsShape::FromJson(j["Shape"]);
		if (!m_Shape) {
			return;
		}
	}

	if (j.find("Position") == j.end()) return;

	const b2Vec2 position = b2Vec2(j["Position"][0], j["Position"][1]);

	if (j.find("Angle") == j.end()) return;

	const float angle = j["Angle"];

	b2BodyType bodyType = b2_staticBody;

	// BodyType is soft. If it's not there we'll just use the default.
	if (j.find("BodyType") != j.end()) {
		if (j["BodyType"].is_string()) {
			const std::string bodyTypeStr = j["BodyType"];
			if (bodyTypeStr == "Static") {
				bodyType = b2_staticBody;
			}
			else if (bodyTypeStr == "Dynamic") {
				bodyType = b2_dynamicBody;
			}
			else if (bodyTypeStr == "Kinematic") {
				bodyType = b2_kinematicBody;
			}
		}
	}

	// Also soft: fixed rotation-ness
	bool fixedRotation = false;
	if (j.find("FixedRotation") != j.end() &&
		j["FixedRotation"].is_boolean())
	{
		fixedRotation = j["FixedRotation"];
	}

	float linearDamping = 0.0f;
	if (j.find("LinearDamping") != j.end() &&
		j["LinearDamping"].is_number())
	{
		linearDamping = j["LinearDamping"];
	}

	bool isBullet = false;
	if (j.find("IsBullet") != j.end() &&
		j["IsBullet"].is_boolean())
	{
		isBullet = j["IsBullet"];
	}

	fixtureDef.shape = m_Shape.get();
	fixtureDef.density = 1.0f;

	if (j.find("Friction") != j.end() &&
		j["Friction"].is_number())
	{
		fixtureDef.friction = j["Friction"];
	}

	if (j.find("Restitution") != j.end() &&
		j["Restitution"].is_number())
	{
		fixtureDef.restitution = j["Restitution"];
	}

	bodyDef.type = bodyType;
	bodyDef.position = position;
	bodyDef.angle = angle;
	bodyDef.fixedRotation = fixedRotation;
	bodyDef.linearDamping = linearDamping;
	bodyDef.bullet = isBullet;

	if (j.find("AngularDamping") != j.end() &&
		j["AngularDamping"].is_number())
	{
		bodyDef.angularDamping = j["AngularDamping"];
	}
}

bool PhysicsComponentDef::VerifyJson(const nlohmann::json & j) {
	auto log = spdlog::get("console");
	assert(log);

	const char* logCtx = "PhysicsComponentDef::VerifyJson: ";

	if (!j.is_object()) {
		return false;
	}

	if (j.find("Position") == j.end()) return false;
	if (!j["Position"].is_array()) return false;
	if (j["Position"].size() != 2) return false;
	if (!j["Position"][0].is_number()) return false;
	if (!j["Position"][1].is_number()) return false;

	if (j.find("Angle") == j.end()) return false;
	if (!j["Angle"].is_number()) return false;

	if (j.find("Shape") == j.end()) return false;
	if (!PhysicsShape::VerifyJson(j["Shape"])) return false;

	// Warn if there's something off about BodyType.
	if (j.find("BodyType") == j.end()) {
		log->warn("{} Could not find 'BodyType' field.", logCtx);
	}
	else {
		if (!j["BodyType"].is_string()) {
			log->warn("{} 'BodyType' is not a string.", logCtx);
		}
		else {
			std::string bodyType = j["BodyType"];
			std::string validBodyTypes[3] = { "Static", "Dynamic", "Kinematic" };
			bool bodyTypeIsValid = false;
			for (auto valid : validBodyTypes) {
				if (bodyType == valid) {
					bodyTypeIsValid = true;
					break; // Success, BodyType is valid.
				}
			}
			if (!bodyTypeIsValid) {
				log->warn(
					"{} 'BodyType' is not '{}', '{}' or '{}'",
					logCtx,
					validBodyTypes[0],
					validBodyTypes[1],
					validBodyTypes[2]);
			}
		}
	}

	// TODO: Add warnings for missing/bogus FixedRotation.

	// TODO: Add warnings for missing/bogus LinearDamping.

	// TODO: Add warnings for missing/bogus AngularDamping.

	// TODO: Add warnings for missing/bogus Friction.

	// TODO: Add warnings for missing/bogus Restitution.

	return true;
}

}