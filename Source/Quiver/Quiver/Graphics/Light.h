#pragma once

#include <Box2D/Common/b2Math.h>
#include <SFML/Graphics/Color.hpp>
#include <json.hpp>

namespace qvr {

struct AmbientLight {
	sf::Color mColor = sf::Color(64, 64, 64, 255);
};

nlohmann::json ToJson(const AmbientLight& light);
AmbientLight   FromJson(const nlohmann::json& j);

class DirectionalLight {
public:
	b2Vec3 GetDirection() const { return mDirection; }

	sf::Color GetColor() const { return mColor; }

	void SetDirection(b2Vec3 lightDirection) {
		lightDirection.Normalize();
		mDirection = lightDirection;
	}

	void SetDirection(const float lightAngleRadians) {
		SetAngleHorizontal(lightAngleRadians);
	}

	float GetAngleHorizontal() const {
		return atan2f(mDirection.y, mDirection.x);
	}

	float GetAngleVertical() const {
		return atan2f(mDirection.z, mDirection.x);
	}

	void SetAngleHorizontal(const float lightAngleRadians);

	void SetAngleVertical(const float lightAngleRadians);

	void SetColor(const sf::Color color) {
		mColor = sf::Color(color.r, color.g, color.b, mColor.a);
	}

	bool ToJson(nlohmann::json& j) const;
	bool FromJson(const nlohmann::json& j);

	void GuiControls();

private:
	b2Vec3 mDirection = b2Vec3(1.0f, 0.0f, 0.0f);

	sf::Color mColor = sf::Color(128, 128, 128, 255);
};

}