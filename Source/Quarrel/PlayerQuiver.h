#pragma once

#include <array>

#include <optional.hpp>

#include <SFML/Graphics/Color.hpp>

#include "CrossbowBolt.h"

struct QuarrelTypeInfo
{
	sf::Color colour;
	CrossbowBoltEffect effect;
};

class PlayerQuiver {
public:
	static const int MaxEquippedQuarrelTypes = 3;

	using QuarrelSlot = std::experimental::optional<QuarrelTypeInfo>;

	std::array<QuarrelSlot, MaxEquippedQuarrelTypes> quarrelSlots;
};