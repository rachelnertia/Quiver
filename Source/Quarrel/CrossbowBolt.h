#pragma once

#include <chrono>

#include <json.hpp>

#include <SFML/Graphics/Color.hpp>

#include <Quiver/Entity/CustomComponent/CustomComponent.h>

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

inline bool operator==(CrossbowBoltEffect const& a, CrossbowBoltEffect const& b) {
	return
		a.immediateDamage == b.immediateDamage &&
		a.appliesEffect == b.appliesEffect &&
		a.specialEffect == b.specialEffect;
}

inline bool operator!=(CrossbowBoltEffect const& a, CrossbowBoltEffect const& b) {
	return !(a == b);
}

void to_json  (nlohmann::json&,       CrossbowBoltEffect const&);
void from_json(nlohmann::json const&, CrossbowBoltEffect&);

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