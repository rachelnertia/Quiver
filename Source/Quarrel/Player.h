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

class Weapon;

class Player : public qvr::CustomComponent {
public:
	Player(qvr::Entity& entity);
	~Player();

	void HandleInput(qvr::RawInputDevices& inputDevices, const float deltaSeconds) override;

	void OnStep(float timestep) override;

	void OnBeginContact(qvr::Entity& other) override;
	void OnEndContact  (qvr::Entity& other) override;

	std::string GetTypeName() const override { return "Player"; }

	void GUIControls() override;

	nlohmann::json ToJson() const override;
	bool FromJson(const nlohmann::json& j) override;

	qvr::Camera3D mCamera;

private:
	void RenderCurrentWeapon(sf::RenderTarget& target) const;

	float mMoveSpeed = 1.0f;
	float mDamage = 0.0f;
	
	std::unique_ptr<Weapon> mCurrentWeapon;

	bool mHideWeaponButtonWasPressed = false;
};