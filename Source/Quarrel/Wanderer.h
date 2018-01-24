#pragma once

#include "Quiver/Entity/CustomComponent/CustomComponent.h"

#include <Box2D/Common/b2Math.h>

// This CustomComponent class exists purely for testing purposes.
class Wanderer : public qvr::CustomComponent{
public:
	Wanderer(qvr::Entity& entity);
	~Wanderer();

	void OnStep(float timestep) override;

	void OnBeginContact(qvr::Entity& other) override;
	void OnEndContact  (qvr::Entity& other) override;

	std::string GetTypeName() const override { return "Wanderer"; }

private:
	enum class State {
		Turning,
		Walking
	};

	State mState = State::Walking;

	struct WalkingState {
		b2Vec2 startPos;
		b2Vec2 direction;
	};

	WalkingState mWalkingState;

	struct TurningState {
		float startAngle = 0.0f;
		float currentAngle = 0.0f;
	};

	TurningState mTurningState;
};