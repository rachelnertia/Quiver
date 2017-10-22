#pragma once

#include <algorithm>
#include <json.hpp>

#include <SFML/Graphics/Color.hpp>

namespace qvr {

class Fog {
public:
	sf::Color GetColor() const { return color; }
	// Distance at which attenuation will begin.
	float GetMinDistance() const { return minDistance; }
	// Distance at which attenuation will end.
	float GetMaxDistance() const { return maxDistance; }
	float GetAttenuationRange() const { return GetMaxDistance() - GetMinDistance(); }
	// [0, 1]
	float GetMaxIntensity() const { return maxIntensity; }
	// [0, 1]
	float GetIntensity(const float distance) const {
		return
			std::min(
			((std::min(
				std::max(distance, minDistance),
				maxDistance)
				- minDistance)
				/ GetAttenuationRange()),
				GetMaxIntensity());
	}

	void SetColor(const sf::Color newColor) {
		this->color = sf::Color(newColor.r, newColor.g, newColor.b, 0);
	}
	void SetMaxIntensity(const float intensity) {
		maxIntensity = std::min(std::max(intensity, 0.0f), 1.0f);
	}
	void SetMinDistance(const float distance) {
		minDistance = std::max(std::min(distance, maxDistance), 0.0f);
	}
	void SetMaxDistance(const float distance) {
		maxDistance = std::max(distance, minDistance);
	}

	static Fog FromJson(const nlohmann::json& j);

private:
	sf::Color color = sf::Color(255, 255, 255, 0);
	float minDistance = 1.0f;
	float maxDistance = 20.0f;
	float maxIntensity = 1.0f;
};

void GuiControls(Fog& distanceShade);

nlohmann::json ToJson(const Fog& fog);

}