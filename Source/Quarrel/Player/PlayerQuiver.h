#pragma once

#include <array>
#include <chrono>

#include <optional.hpp>

#include <SFML/Graphics/Color.hpp>

#include <Quiver/Graphics/ColourUtils.h>

#include "CrossbowBolt.h"

struct QuarrelTypeInfo
{
	std::string name;
	std::chrono::duration<float> cooldownTime;
	sf::Color colour;
	CrossbowBoltEffect effect;
};

void to_json(nlohmann::json& j, const QuarrelTypeInfo& quarrelType);
void from_json(const nlohmann::json& j, QuarrelTypeInfo& quarrelType);

class QuarrelSlot
{
	using duration = std::chrono::duration<float>;
	
	duration cooldownRemaining;
	duration cooldownTime;

public:
	auto GetCooldownRatio() const -> float {
		return 
			cooldownTime != duration(0) ?
				cooldownRemaining / cooldownTime :
				0.0f;
	}

	auto TakeQuarrel()
		-> std::experimental::optional<QuarrelTypeInfo>;
	auto TakeQuarrel(const duration cooldown)
		-> std::experimental::optional<QuarrelTypeInfo>;

	void ResetCooldown() {
		cooldownRemaining = duration(0);
		//cooldownTime = duration(0);
	}

	void OnStep(const std::chrono::duration<float> deltaTime) {
		cooldownRemaining = std::max(cooldownRemaining - deltaTime, duration(0));
	}

	QuarrelTypeInfo type;

	QuarrelSlot(const QuarrelTypeInfo& type) : type(type) {}
};

inline bool CanTakeQuarrel(const QuarrelSlot& slot) {
	return slot.GetCooldownRatio() <= 0.0f;
}

class PlayerQuiver {
public:
	static const int MaxEquippedQuarrelTypes = 3;

	using OptionalQuarrelSlot = std::experimental::optional<QuarrelSlot>;
	using Slots = std::array<OptionalQuarrelSlot, MaxEquippedQuarrelTypes>;

	Slots quarrelSlots;

	void OnStep(const std::chrono::duration<float> deltaTime) {
		for (auto& quarrelSlot : quarrelSlots) {
			if (quarrelSlot) {
				quarrelSlot->OnStep(deltaTime);
			}
		}
	}
};

using OptionalQuarrelType = std::experimental::optional<QuarrelTypeInfo>;

auto TakeQuarrel   (PlayerQuiver& quiver, const int slotIndex) -> OptionalQuarrelType;
void PutQuarrelBack(PlayerQuiver& quiver, const QuarrelTypeInfo& quarrel);

void to_json  (nlohmann::json& j,       PlayerQuiver const& quiver); 
void from_json(nlohmann::json const& j, PlayerQuiver & quiver);