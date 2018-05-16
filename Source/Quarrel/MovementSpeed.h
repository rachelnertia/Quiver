#pragma once

#include <algorithm>

class MovementSpeed {
	float baseSpeed = 0.0f;
	float multiplier = 1.0f;
public:
	MovementSpeed() = default;
	MovementSpeed(const float base)
		: baseSpeed(base)
	{}
	MovementSpeed(const float base, const float multiplier) 
		: baseSpeed(base)
		, multiplier(multiplier)
	{}
	MovementSpeed(const MovementSpeed&) = default;

	float Get() const { return baseSpeed * multiplier; }

	float GetMultiplier() const { return multiplier; }
	
	void SetMultiplier(const float newMultiplier) { 
		multiplier = std::max(0.0f, newMultiplier); 
	}

	void ResetMultiplier() { multiplier = 1.0f; }

	void SetBase(const float newBase) {
		baseSpeed = std::max(0.0f, newBase);
	}
};