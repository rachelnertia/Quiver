#include "ContactFilter.h"

#include <Box2D/Dynamics/b2Fixture.h>

#include "Quiver/Entity/PhysicsComponent/PhysicsComponent.h"

namespace {

// TODO: Move this into a utilties module.
qvr::PhysicsComponent* GetPhysicsComponentFromFixture(b2Fixture& fixture)
{
	return static_cast<qvr::PhysicsComponent*>(fixture.GetBody()->GetUserData());
}

}

namespace qvr {

namespace Physics {

bool ContactFilter::ShouldCollide(b2Fixture* fixtureA, b2Fixture* fixtureB)
{
	const auto physicsComponentA = GetPhysicsComponentFromFixture(*fixtureA);
	const auto physicsComponentB = GetPhysicsComponentFromFixture(*fixtureB);

	if (physicsComponentA && physicsComponentB)
	{
		// Determine if the Entities do not overlap in the Z axis.

		const float aTop = physicsComponentA->GetTop();
		const float aBottom = physicsComponentA->GetBottom();
		const float bTop = physicsComponentB->GetTop();
		const float bBottom = physicsComponentB->GetBottom();

		if (aBottom > bTop)
		{
			// A is above B.
			return false;
		}

		if (aTop < bBottom)
		{
			// A is below B.
			return false;
		}
	}

	return b2ContactFilter::ShouldCollide(fixtureA, fixtureB);
}

}

}