#include "PhysicsComponent.h"

#include <Box2D/Dynamics/b2Body.h>
#include <Box2D/Dynamics/b2Fixture.h>
#include <Box2D/Dynamics/b2World.h>
#include <spdlog/spdlog.h>

#include "Quiver/Entity/Entity.h"
#include "Quiver/Entity/PhysicsComponent/PhysicsComponentDef.h"
#include "Quiver/Physics/PhysicsShape.h"
#include "Quiver/World/World.h"

namespace qvr {

PhysicsComponent::PhysicsComponent(Entity& entity, const PhysicsComponentDef& def)
	: Component(entity)
{
	auto log = spdlog::get("console");
	assert(log);

	{
		b2Body* body = const_cast<b2World*>(GetEntity().GetWorld().GetPhysicsWorld())->CreateBody(&def.bodyDef);
		if (!body) {
			log->error("Failed to create body!");
		}

		// Point the b2Body to this PhysicsComponent.
		body->SetUserData(this);

		mBody.reset(body);
	}

	{
		// Attach the fixture to the body.
		b2Fixture* f = nullptr;
		f = mBody->CreateFixture(&def.fixtureDef);
		if (!f) {
			log->error("Failed to create fixture!");
		}
	}
}

PhysicsComponent::~PhysicsComponent()
{
	assert(mBody->GetUserData() == this);
	// We do this here to avoid a crash in ContactListener.
	mBody->SetUserData(nullptr);
};

namespace
{

const b2Fixture& GetLastFixtureInList(const b2Fixture& fixture) {
	const b2Fixture* last = &fixture;
	const b2Fixture* next = fixture.GetNext();
	while (next != nullptr) {
		last = next;
		next = next->GetNext();
	}
	return *last;
}

}

nlohmann::json PhysicsComponent::ToJson()
{
	auto log = spdlog::get("console");
	assert(log);

	using json = nlohmann::json;

	if (!mBody) {
		log->error("Trying to serialize a body-less PhysicsComponent.");
		return {};
	}

	json j;

	j["Position"][0] = mBody->GetPosition().x;
	j["Position"][1] = mBody->GetPosition().y;
	j["Angle"] = mBody->GetAngle();
	j["FixedRotation"] = mBody->IsFixedRotation();
	j["IsBullet"] = mBody->IsBullet();
	j["LinearDamping"] = mBody->GetLinearDamping();
	j["AngularDamping"] = mBody->GetAngularDamping();

	switch (mBody->GetType()) {
	case b2_staticBody:
		j["BodyType"] = "Static";
		break;
	case b2_dynamicBody:
		j["BodyType"] = "Dynamic";
		break;
	case b2_kinematicBody:
		j["BodyType"] = "Kinematic";
		break;
	default:
		log->warn("mBody->GetType() returned garbage. Serializing as Static.");
		j["BodyType"] = "Static";
		break;
	}

	// At the moment I don't support serialization for bodies with multiple fixtures, 
	// fixtures with multiple shapes, or shapes with child shapes.
	// We grab the last fixture on the list, which is the fixture the body started with.
	const b2Fixture& fixture = GetLastFixtureInList(*mBody->GetFixtureList());

	j["Friction"] = fixture.GetFriction();
	j["Restitution"] = fixture.GetRestitution();

	{
		json shapeData = PhysicsShape::ToJson(*fixture.GetShape());
		if (shapeData.empty()) {
			log->error("Couldn't serialize shape.");
			return {};
		}

		j["Shape"] = shapeData;
	}

	{
		// Do fixture data... which we don't care about yet.
	}

	return j;
}

b2Vec2 PhysicsComponent::GetPosition() const
{
	if (mBody)
	{
		return mBody->GetPosition();
	}

	return b2Vec2_zero;
}

}