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