#include <catch.hpp>

#include <Box2D/Collision/Shapes/b2CircleShape.h>
#include <Box2D/Dynamics/b2World.h>

#include "Quiver/Entity/Entity.h"
#include "Quiver/Entity/CustomComponent/CustomComponent.h"
#include "Quiver/Entity/PhysicsComponent/PhysicsComponent.h"
#include "Quiver/Entity/PhysicsComponent/PhysicsComponentDef.h"
#include "Quiver/World/World.h"

using namespace qvr;

TEST_CASE("PhysicsComponent creation and cleanup", "[Physics]")
{
	CustomComponentTypeLibrary types;
	FixtureFilterBitNames filterBitNames;

	WorldContext worldContext(types, filterBitNames);

	World world(worldContext);

	auto entity = [&]() {
		return std::make_unique<Entity>(
			world, 
			PhysicsComponentDef(b2CircleShape(), b2Vec2_zero, 0.0f));
	}(); 

	REQUIRE(entity->GetPhysics()->GetBody().GetWorld() == world.GetPhysicsWorld());

	REQUIRE(world.GetPhysicsWorld()->GetBodyCount() == 1);

	entity.reset();

	REQUIRE(world.GetPhysicsWorld()->GetBodyCount() == 0);
}