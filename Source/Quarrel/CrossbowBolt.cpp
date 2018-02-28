#include "CrossbowBolt.h"

#include <Box2D/Dynamics/b2Body.h>
#include <Box2D/Dynamics/b2Fixture.h>
#include <Quiver/Entity/PhysicsComponent/PhysicsComponent.h>
#include <Quiver/Entity/Entity.h>

#include "Utils.h"

using namespace qvr;

CrossbowBolt::CrossbowBolt(Entity & entity)
	: CustomComponent(entity)
{
	b2Fixture& fixture = *GetEntity().GetPhysics()->GetBody().GetFixtureList();
	b2Filter filter = fixture.GetFilterData();
	filter.categoryBits = FixtureFilterCategories::CrossbowBolt;
}

void CrossbowBolt::OnBeginContact(Entity & other, b2Fixture & myFixture)
{
	this->SetRemoveFlag(true);
}
