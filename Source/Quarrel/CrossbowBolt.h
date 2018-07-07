#pragma once

#include <chrono>

#include <json.hpp>

#include <SFML/Graphics/Color.hpp>

#include <Quiver/Entity/CustomComponent/CustomComponent.h>

#define BETTER_ENUMS_DEFAULT_CONSTRUCTOR(Enum) \
  public:                                      \
    Enum() = default;

#include "enum.h"

#include "Effects.h"
#include "Utils.h"

BETTER_ENUM(
	SpecialEffectType, int,
	None,
	Teleport);

struct CrossbowBoltEffect
{
	int immediateDamage = 0;
	ActiveEffectType appliesEffect = ActiveEffectType::None;
	SpecialEffectType specialEffect = SpecialEffectType::None;
};

void to_json  (nlohmann::json&,       CrossbowBoltEffect const&);
void from_json(nlohmann::json const&, CrossbowBoltEffect&);

void to_json(nlohmann::json&, SpecialEffectType const&);
void from_json(nlohmann::json const&, SpecialEffectType&);

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