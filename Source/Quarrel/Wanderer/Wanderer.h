#pragma once

#include <Quiver/Entity/EntityId.h>
#include <Quiver/Entity/CustomComponent/CustomComponent.h>

#include <Box2D/Common/b2Math.h>

class b2Fixture;

// This CustomComponent class exists purely for testing purposes.
class Wanderer : public qvr::CustomComponent{
public:
	Wanderer(qvr::Entity& entity);
	~Wanderer();

	void OnStep(const std::chrono::duration<float> timestep) override;

	void OnBeginContact(
		qvr::Entity& other, 
		b2Fixture& myFixture, 
		b2Fixture& otherFixture) override;
	void OnEndContact(
		qvr::Entity& other, 
		b2Fixture& myFixture, 
		b2Fixture& otherFixture) override;

	std::string GetTypeName() const override { return "Wanderer"; }

private:
	b2Fixture* m_Sensor = nullptr;
	b2Vec2 m_WalkDirection;
	qvr::EntityId m_PlayerInSensor = qvr::EntityId(0);
};