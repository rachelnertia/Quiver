#include "ContactListener.h"

#include <Box2D/Dynamics/Contacts/b2Contact.h>

#include "Quiver/Entity/Entity.h"
#include "Quiver/Entity/CustomComponent/CustomComponent.h"
#include "Quiver/Entity/PhysicsComponent/PhysicsComponent.h"

namespace {

// TODO: Move this into a utilities module.
qvr::Entity* GetEntityFromFixture(const b2Fixture& fixture) {
	auto physicsComp
		= static_cast<qvr::PhysicsComponent*>(fixture.GetBody()->GetUserData());

	if (physicsComp) {
		return &physicsComp->GetEntity();
	}

	return nullptr;
}

// TODO: Move this into a utilties module.
qvr::PhysicsComponent* GetPhysicsComponentFromFixture(b2Fixture& fixture)
{
	return static_cast<qvr::PhysicsComponent*>(fixture.GetBody()->GetUserData());
}

}

namespace qvr {

namespace Physics
{

void ContactListener::BeginContact(b2Contact * contact)
{
	Entity* entityA
		= GetEntityFromFixture(*contact->GetFixtureA());
	Entity* entityB
		= GetEntityFromFixture(*contact->GetFixtureB());
	if (entityA && entityB) {
		if (entityA->GetCustomComponent()) {
			entityA->GetCustomComponent()->OnBeginContact(
				*entityB, 
				*contact->GetFixtureA(), 
				*contact->GetFixtureB());
		}
		if (entityB->GetCustomComponent()) {
			entityB->GetCustomComponent()->OnBeginContact(
				*entityA, 
				*contact->GetFixtureB(), 
				*contact->GetFixtureA());
		}
	}
}

void ContactListener::EndContact(b2Contact * contact)
{
	Entity* entityA
		= GetEntityFromFixture(*contact->GetFixtureA());
	Entity* entityB
		= GetEntityFromFixture(*contact->GetFixtureB());
	if (entityA && entityB) {
		if (entityA->GetCustomComponent()) {
			entityA->GetCustomComponent()->OnEndContact(
				*entityB, 
				*contact->GetFixtureA(),
				*contact->GetFixtureB());
		}
		if (entityB->GetCustomComponent()) {
			entityB->GetCustomComponent()->OnEndContact(
				*entityA, 
				*contact->GetFixtureB(),
				*contact->GetFixtureA());
		}
	}
}

void ContactListener::PreSolve(b2Contact* contact, const b2Manifold* oldManifold)
{
	// TODO: Rewrite this so it works regardless of the A-B ordering of fixtures.

	PhysicsComponent* physicsComponentA = GetPhysicsComponentFromFixture(*contact->GetFixtureB());
	PhysicsComponent* physicsComponentB = GetPhysicsComponentFromFixture(*contact->GetFixtureA());

	if (physicsComponentA && physicsComponentB)
	{
		// Determine if the Entities do not overlap in the Z axis.

		const float aTop = physicsComponentA->GetTop();
		const float aBottom = physicsComponentA->GetBottom();
		const float bTop = physicsComponentB->GetTop();
		const float bBottom = physicsComponentB->GetBottom();

		if (aBottom >= bTop)
		{
			// A is above B.
			contact->SetEnabled(false);

			if (physicsComponentA->GetNextBottom() < bTop)
			{
				physicsComponentA->SetBottom(bTop);
				// Set velocity to 0.
				physicsComponentA->SetGrounded(contact->GetFixtureA());
			}

			return;
		}

		if (aTop <= bBottom)
		{
			// A is below B.
			contact->SetEnabled(false);

			if (physicsComponentA->GetNextTop() > bBottom)
			{
				physicsComponentA->SetTop(bBottom);
				physicsComponentA->SetZVelocity(0.0f);
			}

			return;
		}
	}
}

}

}