#pragma once

#include "Quiver/Entity/CustomComponent/CustomComponent.h"
#include "Quiver/Graphics/Camera3D.h"

#include "Weapon.h"

namespace sf {
class RenderTarget;
}

namespace qvr {
class Entity;
}

class PlayerEditor;
class Weapon;

class Player : public qvr::CustomComponent {
public:
	Player(qvr::Entity& entity);
	~Player();

	void HandleInput(qvr::RawInputDevices& inputDevices, const float deltaSeconds) override;

	void OnStep(float timestep) override;

	void OnBeginContact(qvr::Entity& other, b2Fixture& myFixture) override;
	void OnEndContact  (qvr::Entity& other, b2Fixture& myFixture) override;

	std::string GetTypeName() const override { return "Player"; }

	std::unique_ptr<qvr::CustomComponentEditor> CreateEditor() override;

	nlohmann::json ToJson() const override;
	bool FromJson(const nlohmann::json& j) override;

	qvr::Camera3D mCamera;

	friend class PlayerEditor;

private:
	void RenderCurrentWeapon(sf::RenderTarget& target) const;
	void RenderHud          (sf::RenderTarget& target) const;

	float mMoveSpeed = 1.0f;
	float mDamage = 0.0f;
	
	std::unique_ptr<Weapon> mCurrentWeapon;

	bool mCannotDie = false;
};