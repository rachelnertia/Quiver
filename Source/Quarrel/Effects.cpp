#include "Effects.h"

#include <algorithm>
#include <cassert>

#include <Quiver/Entity/RenderComponent/RenderComponent.h>

#include "Damage.h"
#include "MovementSpeed.h"

using namespace std::chrono_literals;

void AddActiveEffect(const ActiveEffectType effectType, ActiveEffectSet& activeEffects)
{
	using namespace std;

	auto AddOrResetDuration = [&](const std::chrono::duration<float> duration) {
		auto it = find_if(
			begin(activeEffects.container),
			end(activeEffects.container),
			[effectType](const auto& effect) { return effectType == effect.type; });
		if (it == end(activeEffects.container)) {
			activeEffects.container.push_back({ effectType, duration, 0s });
		}
		else {
			it->remainingDuration = duration;
		}
	};

	switch (effectType)
	{
	case ActiveEffectType::None: return;
	case ActiveEffectType::Poisoned:
	{
		const auto poisonDuration = 10s;
		AddOrResetDuration(poisonDuration);
		break;
	}
	case ActiveEffectType::Burning:
	{
		const auto burningDuration = 10s;
		AddOrResetDuration(burningDuration);
		break;
	}
	case ActiveEffectType::Frozen:
	{
		// TODO: Fun stuff with interactions between Frozen/Wet and Burning.

		const auto frozenDuration = 10s;
		AddOrResetDuration(frozenDuration);
		break;
	}
	}
}

void RemoveExpiredEffects(ActiveEffectSet& effects)
{
	effects.container.erase(
		std::remove_if(
			std::begin(effects.container),
			std::end(effects.container),
			[](const ActiveEffect& effect) { return effect.remainingDuration <= 0s; }),
		std::end(effects.container));
}

void ApplyEffect(const ActiveEffect & activeEffect, DamageCount & damage)
{
	switch (activeEffect.type)
	{
	case ActiveEffectType::None: assert(false); break;
	case ActiveEffectType::Burning:
		AddDamage(damage, 1);
		break;
	case ActiveEffectType::Poisoned:
		AddDamage(damage, 1);
		break;
	}
}

void ApplyEffect(const ActiveEffect& effect, MovementSpeed& speed)
{
	assert(effect.type != ActiveEffectType::None);

	if (effect.type == ActiveEffectType::Chilled) {
		speed.SetMultiplier(0.5f);
	}
	else if (effect.type == ActiveEffectType::Frozen) {
		speed.SetMultiplier(0.0f);
	}
}

// Returns true if the damage effect should be applied.
bool UpdateEffect(ActiveEffect & activeEffect, const std::chrono::duration<float> deltaTime)
{
	activeEffect.remainingDuration -= deltaTime;

	const auto oldRunningDuration = activeEffect.runningDuration;
	activeEffect.runningDuration += deltaTime;

	// Only apply damage effects when we cross over a second.
	const float x = ceil(oldRunningDuration.count());
	return activeEffect.runningDuration.count() > x && oldRunningDuration.count() <= x;
}

void ApplyEffect(const ActiveEffect & effect, qvr::RenderComponent & renderComponent)
{
	auto CalculatePulseColour =
		[](const std::chrono::duration<float> timeLeft, const sf::Color pulseColour)
	{
		const float seconds = timeLeft.count();
		const sf::Uint8 s = 
			floor(seconds) > 0.0f ? 
			(sf::Uint8)(255.0f * abs(seconds - round(seconds))) : 
			(sf::Uint8)(255.0f * abs(1.0f - seconds));
		const sf::Uint8 r = std::max(pulseColour.r, s);
		const sf::Uint8 g = std::max(pulseColour.g, s);
		const sf::Uint8 b = std::max(pulseColour.b, s);
		return sf::Color(r, g, b);
	};

	switch (effect.type)
	{
	case ActiveEffectType::None: assert(false); break;
	case ActiveEffectType::Burning:
	{
		renderComponent.SetColor(
			CalculatePulseColour(
				effect.remainingDuration,
				sf::Color::Red));
		break;
	}
	case ActiveEffectType::Poisoned:
	{
		renderComponent.SetColor(
			CalculatePulseColour(
				effect.remainingDuration,
				sf::Color::Green));
		break;
	}
	case ActiveEffectType::Frozen:
		renderComponent.SetColor(
			CalculatePulseColour(
				effect.remainingDuration,
				sf::Color::Blue));
	}
}
