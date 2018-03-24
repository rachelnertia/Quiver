#pragma once

#include <chrono>

#include <SFML/Graphics/Color.hpp>

#include <Quiver/Entity/CustomComponent/CustomComponent.h>

#include "Utils.h"

enum class ActiveEffectType
{
	None,
	Burning,
	Poisoned
};

struct CrossbowBoltEffect
{
	float immediateDamage = 0.0f;
	ActiveEffectType appliesEffect = ActiveEffectType::None;
};

class CrossbowBolt : public qvr::CustomComponent
{
public:
	CrossbowBolt(qvr::Entity& entity);

	void OnBeginContact(
		qvr::Entity& other, 
		b2Fixture& myFixture,
		b2Fixture& otherFixture) override;

	void OnStep(const std::chrono::duration<float> deltaTime) override;

	std::string GetTypeName() const override { return "CrossbowBolt"; }

	CrossbowBoltEffect effect;
	EntityRef shooter;
	bool collided = false;
};