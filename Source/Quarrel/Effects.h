#pragma once

#include <chrono>
#include <vector>

namespace qvr
{
class RenderComponent;
}

enum class ActiveEffectType
{
	None,
	Burning,
	Poisoned
};

struct ActiveEffect
{
	ActiveEffectType type;
	std::chrono::duration<float> remainingDuration;
	std::chrono::duration<float> runningDuration;
};

void AddActiveEffect(
	const ActiveEffectType effectType,
	std::vector<ActiveEffect>& activeEffects);

// Returns true if the damage effect should be applied this frame.
bool UpdateEffect(ActiveEffect& activeEffect, const std::chrono::duration<float> deltaTime);

void ApplyEffect(const ActiveEffect& activeEffect, int& damage);
void ApplyEffect(const ActiveEffect& effect, qvr::RenderComponent& renderComponent);