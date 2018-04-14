#pragma once

#include <algorithm>

struct DamageCount
{
	DamageCount(const int maximum) : max(maximum) {}

	int damage = 0;
	int max;
};

inline void AddDamage(DamageCount& counter, const int amount) {
	counter.damage = std::min(counter.max, counter.damage + amount);
}

inline void RemoveDamage(DamageCount& counter, const int amount) {
	counter.damage = std::min(counter.max, counter.damage - amount);
}

inline bool HasTakenDamage(const DamageCount& counter) {
	return counter.damage > 0;
}

inline bool HasExceededLimit(const DamageCount& counter) {
	return counter.damage >= counter.max;
}