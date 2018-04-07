#include "EnemyMelee.h"

#include <Box2D/Collision/Shapes/b2CircleShape.h>
#include <Box2D/Dynamics/b2Body.h>
#include <Box2D/Dynamics/b2Fixture.h>

#include <Quiver/Entity/Entity.h>
#include <Quiver/Entity/CustomComponent/CustomComponent.h>
#include <Quiver/Entity/PhysicsComponent/PhysicsComponent.h>

#include "Utils.h"

class EnemyMelee : public qvr::CustomComponent
{
	b2Fixture* sensorFixture = nullptr;
	
	EntityRef target;

public:
	EnemyMelee(qvr::Entity& entity);

	std::string GetTypeName() const override {
		return "EnemyMelee";
	}

	void OnStep(const std::chrono::duration<float> deltaTime) override;

	void OnBeginContact(
		qvr::Entity& other,
		b2Fixture& myFixture,
		b2Fixture& otherFixture) override;

	void OnEndContact(
		qvr::Entity& other,
		b2Fixture& myFixture,
		b2Fixture& otherFixture) override;
};

std::unique_ptr<qvr::CustomComponent> CreateEnemyMelee(qvr::Entity& entity) {
	return std::make_unique<EnemyMelee>(entity);
}

EnemyMelee::EnemyMelee(qvr::Entity& entity)
	: CustomComponent(entity)
{
	if (GetEntity().GetPhysics()) {
		SetCategoryBits(
			*GetEntity().GetPhysics()->GetBody().GetFixtureList(),
			FixtureFilterCategories::Enemy);

		// Create sensor fixture.
		b2FixtureDef fixtureDef;
		b2CircleShape circleShape = CreateCircleShape(5.0f);
		fixtureDef.shape = &circleShape;
		fixtureDef.isSensor = true;
		fixtureDef.filter.categoryBits = FixtureFilterCategories::Sensor;
		// Only collide with Players.
		fixtureDef.filter.maskBits = FixtureFilterCategories::Player;
		sensorFixture = GetEntity().GetPhysics()->GetBody().CreateFixture(&fixtureDef);
	}
}

void EnemyMelee::OnBeginContact(
	qvr::Entity& other,
	b2Fixture& myFixture,
	b2Fixture& otherFixture)
{
	if (&myFixture == sensorFixture) {
		if (target.Get() == nullptr) {
			target = EntityRef(other);
		}
	}
}

void EnemyMelee::OnEndContact(
	qvr::Entity& other,
	b2Fixture& myFixture,
	b2Fixture& otherFixture)
{
	if (&myFixture == sensorFixture) {
		if (target.Get() == &other) {
			target = {};
		}
	}
}

void EnemyMelee::OnStep(const std::chrono::duration<float> deltaTime) {
	if (target.Get()) {
		b2Body& body = GetEntity().GetPhysics()->GetBody();
		
		const b2Vec2 targetPosition = target.Get()->GetPhysics()->GetPosition();

		b2Vec2 velocity = targetPosition - body.GetPosition();
		velocity.Normalize();
		velocity *= 1.0f;

		//body.ApplyForceToCenter(force, true);
		body.SetLinearVelocity(velocity);
	}
}