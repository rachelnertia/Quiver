#include "PlayerQuiver.h"

PlayerQuiver::PlayerQuiver() {
	QuarrelTypeInfo type;
	type.colour = sf::Color::Black;
	type.effect.immediateDamage = 5;

	quarrelSlots[0] = type;

	type.colour = sf::Color::Red;
	type.effect.immediateDamage = 1;
	type.effect.appliesEffect = ActiveEffectType::Burning;

	quarrelSlots[1] = type;

	type.colour = sf::Color::White;
	type.effect.appliesEffect = ActiveEffectType::None;
	type.effect.specialEffect = SpecialEffectType::Teleport;

	quarrelSlots[2] = type;
}