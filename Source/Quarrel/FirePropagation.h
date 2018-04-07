#pragma once

#include <vector>

class b2Fixture;

struct ActiveEffectSet;

struct FiresInContact {
	std::vector<const b2Fixture*> container;
};

void OnBeginContact(
	FiresInContact& fires,
	const b2Fixture& fixture);

void OnEndContact(
	FiresInContact& fires,
	const b2Fixture& fixture);

void ApplyFires(FiresInContact& fires, ActiveEffectSet& effects);