#pragma once

#include <Box2D/Common/b2Math.h>
#include <SFML/Graphics/Color.hpp>
#include <json.hpp>

namespace qvr {

struct AmbientLight {
	sf::Color mColor = sf::Color::White;
};

nlohmann::json ToJson(const AmbientLight& light);
AmbientLight   FromJson(const nlohmann::json& j);

class DirectionalLight {
public:
	b2Vec2 GetDirection() const { return mDirection; }

	sf::Color GetColor() const { return mColor; }

	void SetDirection(b2Vec2 lightDirection) {
		if (lightDirection.Length() > 1.0f) {
			lightDirection.Normalize();
		}

		mDirection = lightDirection;
	}

	void SetDirection(const float lightAngleRadians) {
		mDirection.x = cosf(lightAngleRadians);
		mDirection.y = sinf(lightAngleRadians);
	}

	void SetColor(const sf::Color color) {
		mColor = sf::Color(color.r, color.g, color.b, mColor.a);
	}

	bool ToJson(nlohmann::json& j) const;
	bool FromJson(const nlohmann::json& j);

	void GuiControls();

private:
	b2Vec2 mDirection = b2Vec2(1.0f, 0.0f);

	sf::Color mColor = sf::Color(64, 64, 64, 255);
};

}