#include "ContactListener.h"

#include <Box2D/Dynamics/Contacts/b2Contact.h>

#include "Quiver/Entity/Entity.h"
#include "Quiver/Entity/CustomComponent/CustomComponent.h"
#include "Quiver/Entity/PhysicsComponent/PhysicsComponent.h"

namespace {

qvr::Entity* GetEntityFromFixture(const b2Fixture& fixture) {
	auto physicsComp
		= static_cast<qvr::PhysicsComponent*>(fixture.GetBody()->GetUserData());

	if (physicsComp) {
		return &physicsComp->GetEntity();
	}

	return nullptr;
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
			entityA->GetCustomComponent()->OnBeginContact(*entityB, *contact->GetFixtureA());
		}
		if (entityB->GetCustomComponent()) {
			entityB->GetCustomComponent()->OnBeginContact(*entityA, *contact->GetFixtureB());
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
			entityA->GetCustomComponent()->OnEndContact(*entityB, *contact->GetFixtureA());
		}
		if (entityB->GetCustomComponent()) {
			entityB->GetCustomComponent()->OnEndContact(*entityA, *contact->GetFixtureB());
		}
	}
}

}

}