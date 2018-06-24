#include "PlayerQuiver.h"

using namespace std::chrono_literals;

auto QuarrelSlot::TakeQuarrel(const std::chrono::duration<float> cooldown) 
	-> std::experimental::optional<QuarrelTypeInfo>
{
	if (!CanTakeQuarrel(*this)) {
		return {};
	}

	cooldownTime = cooldown;
	cooldownRemaining = cooldown;

	return type;
}

void to_json(nlohmann::json & j, const QuarrelTypeInfo & quarrelType) {
	j = nlohmann::json
	{
		{ "colour", },
	{ "effect", quarrelType.effect }
	};
}

void from_json(const nlohmann::json & j, QuarrelTypeInfo & quarrelType) {
	quarrelType.colour = j.at("colour").get<sf::Color>();
	quarrelType.effect = j.at("effect").get<CrossbowBoltEffect>();
}

auto TakeQuarrel(PlayerQuiver& quiver, const int slotIndex) -> OptionalQuarrelType
{
	assert(slotIndex >= 0);
	assert(slotIndex < PlayerQuiver::MaxEquippedQuarrelTypes);

	if (!quiver.quarrelSlots[slotIndex].has_value()) {
		return {};
	}

	// TODO: Smarter cooldown system.
	const auto basicCooldown = 0.5s;

	return quiver.quarrelSlots[slotIndex]->TakeQuarrel(basicCooldown);
}