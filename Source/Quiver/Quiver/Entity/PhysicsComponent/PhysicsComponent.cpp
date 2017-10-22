#include "PhysicsComponent.h"

#include <Box2D/Dynamics/b2Body.h>
#include <Box2D/Dynamics/b2Fixture.h>
#include <Box2D/Dynamics/b2World.h>
#include <ImGui/imgui.h>

#include "Quiver/Entity/Entity.h"
#include "Quiver/Entity/PhysicsComponent/PhysicsComponentDef.h"
#include "Quiver/Physics/PhysicsShape.h"
#include "Quiver/World/World.h"

namespace qvr {

PhysicsComponent::PhysicsComponent(Entity& entity, const PhysicsComponentDef& def)
	: Component(entity)
{
	{
		b2Body* body = const_cast<b2World*>(GetEntity().GetWorld().GetPhysicsWorld())->CreateBody(&def.bodyDef);
		if (!body) {
			std::cout << "Failed to create body!" << std::endl;
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
			std::cout << "Failed to create fixture!" << std::endl;
		}
	}

}

PhysicsComponent::~PhysicsComponent()
{
	assert(mBody->GetUserData() == this);
	// We do this here to avoid a crash in ContactListener.
	mBody->SetUserData(nullptr);
};

nlohmann::json PhysicsComponent::ToJson()
{
	using json = nlohmann::json;

	if (!mBody) {
		std::cout << "Trying to serialize a body-less PhysicsComponent." << std::endl;
		return {};
	}

	json j;

	// At the moment I don't support bodies with multiple fixtures, fixtures with multiple shapes,
	// or shapes with child shapes.

	j["Position"][0] = mBody->GetPosition().x;
	j["Position"][1] = mBody->GetPosition().y;
	j["Angle"] = mBody->GetAngle();
	j["FixedRotation"] = mBody->IsFixedRotation();
	j["IsBullet"] = mBody->IsBullet();
	j["LinearDamping"] = mBody->GetLinearDamping();
	j["AngularDamping"] = mBody->GetAngularDamping();
	j["Friction"] = mBody->GetFixtureList()->GetFriction();
	j["Restitution"] = mBody->GetFixtureList()->GetRestitution();

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
		std::cout << "mBody->GetType() returned garbage. Serializing as Static." << std::endl;
		j["BodyType"] = "Static";
		break;
	}

	{
		json shapeData = PhysicsShape::ToJson(*mBody->GetFixtureList()->GetShape());
		if (shapeData.empty()) {
			std::cout << "Couldn't serialize shape." << std::endl;
			return {};
		}

		j["Shape"] = shapeData;
	}

	{
		// Do fixture data... which we don't care about yet.
	}

	return j;
}

void PhysicsComponent::GuiControls() {

	{
		b2BodyType type = mBody->GetType();
		const char* bodyTypeNames[3] =
		{
			"Static",    // b2_staticBody
			"Kinematic", // b2_kinematicBody
			"Dynamic"    // b2_dynamicBody
		};

		if (ImGui::ListBox("Body Type",
			(int*)&type,
			bodyTypeNames,
			3))
		{
			mBody->SetType(type);
		}
	}

	{
		bool fixedRotation = mBody->IsFixedRotation();
		if (ImGui::Checkbox("Fixed Rotation", &fixedRotation)) {
			mBody->SetFixedRotation(fixedRotation);
		}
	}

	{
		float linearDamping = mBody->GetLinearDamping();
		if (ImGui::SliderFloat("Linear Damping", &linearDamping, 0.0f, 10.0f)) {
			mBody->SetLinearDamping(linearDamping);
		}
	}

	{
		float angularDamping = mBody->GetAngularDamping();
		if (ImGui::InputFloat("Angular Damping", &angularDamping)) {
			mBody->SetAngularDamping(angularDamping);
		}
	}

	{
		float friction = mBody->GetFixtureList()->GetFriction();
		if (ImGui::SliderFloat("Friction", &friction, 0.0f, 1.0f)) {
			mBody->GetFixtureList()->SetFriction(friction);
		}
	}

	{
		float restitution = mBody->GetFixtureList()->GetRestitution();
		if (ImGui::SliderFloat("Restitution", &restitution, 0.0f, 2.0f)) {
			mBody->GetFixtureList()->SetRestitution(restitution);
		}
	}

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