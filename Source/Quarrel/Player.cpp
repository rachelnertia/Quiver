#include "Player.h"

#include <fstream>
#include <iostream>

#include <Box2D/Common/b2Math.h>
#include <Box2D/Dynamics/b2Body.h>
#include <Box2D/Collision/Shapes/b2CircleShape.h>
#include <Box2D/Collision/Shapes/b2PolygonShape.h>
#include <ImGui/imgui.h>
#include <SFML/Audio.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <spdlog/spdlog.h>

#include "Quiver/Audio/Listener.h"
#include "Quiver/Animation/AnimationSystem.h"
#include "Quiver/Animation/AnimationData.h"
#include "Quiver/Entity/Entity.h"
#include "Quiver/Entity/RenderComponent/RenderComponent.h"
#include "Quiver/Entity/PhysicsComponent/PhysicsComponent.h"
#include "Quiver/Input/BinaryInput.h"
#include "Quiver/Input/Mouse.h"
#include "Quiver/Input/Keyboard.h"
#include "Quiver/Input/RawInput.h"
#include "Quiver/Misc/JsonHelpers.h"
#include "Quiver/World/World.h"

#include "PlayerInput.h"

using namespace qvr;

namespace xb = qvr::Xbox360Controller;

Player::Player(Entity& entity) 
	: CustomComponent(entity)
	, mCurrentWeapon(new Crossbow(*this)) 
{
	GetEntity().GetWorld().RegisterCamera(mCamera);
	
	mCamera.SetPosition(GetEntity().GetPhysics()->GetPosition());

	mCamera.SetOverlayDrawer([this](sf::RenderTarget& target) {
		this->RenderCurrentWeapon(target);
	});
}

Player::~Player() 
{
	GetEntity().GetWorld().UnregisterCamera(mCamera);
}

class DeadPlayer : public CustomComponent
{
public:
	DeadPlayer(Entity& entity, Player& player)
		: CustomComponent(entity)
		, mCamera(player.mCamera)
	{
		entity.GetWorld().RegisterCamera(mCamera);

		if (entity.GetWorld().GetMainCamera() == &player.mCamera) {
			entity.GetWorld().SetMainCamera(mCamera);
		}

		mCamera.SetHeight(0.0f);
	}

	std::string GetTypeName() const override { return "DeadPlayer"; }

	void OnStep(float timestep) override {
		b2Body& body = GetEntity().GetPhysics()->GetBody();

		mCamera.SetPosition(body.GetPosition());
		mCamera.SetRotation(body.GetAngle());

		qvr::UpdateListener(mCamera);
	}

private:
	Camera3D mCamera;

};

void Player::HandleInput(qvr::RawInputDevices& devices, const float deltaSeconds)
{
	using namespace Quarrel;
	
	b2Body& body = GetEntity().GetPhysics()->GetBody();

	// Move.
	{
		const b2Vec2 dir = GetDirectionVector(devices);

		// Don't override the body's velocity if the player isn't providing any input.
		if (dir.LengthSquared() != 0.0f) {
			b2Rot bodyRot(body.GetAngle());

			// Rotate the direction vector into the body's frame.
			b2Vec2 forceDir = b2Mul(bodyRot, dir);

			body.SetLinearVelocity(mMoveSpeed * forceDir);
		}
	}

	// Turn.
	{
		float rotateAngle = GetTurnAngle(devices);
		
		if (rotateAngle != 0.0f) {
			const float rotateSpeed = 3.14f; // radians per second

			const float rotation = 
				rotateAngle * 
				rotateSpeed * 
				deltaSeconds;

			body.SetTransform(body.GetPosition(), body.GetAngle() + rotation);
		}
	}

	if (devices.GetKeyboard().JustDown(qvr::KeyboardKey::Tab)) 
	{
		if (mCurrentWeapon) {
			mCurrentWeapon.release();
		}
		else {
			mCurrentWeapon = std::make_unique<Crossbow>(*this);
		}
	}

	if (mCurrentWeapon != nullptr) {
		mCurrentWeapon->OnStep(devices, deltaSeconds);
	}
}

void Player::OnStep(float deltaSeconds)
{
	auto log = spdlog::get("console");
	assert(log);
	const char* logCtx = "Player::OnStep:";
	
	if (mDamage >= 100.0f) {
		log->debug("{} Oh no! I've taken too much damage!", logCtx);
		
		GetEntity().SetInput(std::make_unique<DeadPlayer>(GetEntity(), *this));

		// Super important to return here!
		return;
	}

	b2Body& body = GetEntity().GetPhysics()->GetBody();

	mCamera.SetPosition(body.GetPosition());
	mCamera.SetRotation(body.GetAngle());

	qvr::UpdateListener(mCamera);
}

void Player::OnBeginContact(Entity& other)
{
	if (other.GetCustomComponent()) {
		auto log = spdlog::get("console");
		assert(log);
		const char* logCtx = "Player::OnBeginContact";

		log->debug("{} Player beginning contact with {}...", logCtx, other.GetCustomComponent()->GetTypeName());

		if (other.GetCustomComponent()->GetTypeName() == "EnemyProjectile") {
			log->debug("{} Player taking damage", logCtx);
			mDamage += 100.0f;
		}
	}
}

void Player::OnEndContact(Entity& other)
{
	auto log = spdlog::get("console");
	assert(log);

	if (other.GetCustomComponent()) {
		log->debug("Player finishing contact with {}...", other.GetCustomComponent()->GetTypeName());
	}
}

void Player::GUIControls()
{
	ImGui::Text("Things will go here...");

	ImGui::SliderFloat("Move Speed", &mMoveSpeed, 0.0f, 20.0f);
}

nlohmann::json Player::ToJson() const
{
	nlohmann::json j;

	j["TestMember"] = mTestMember;
	j["MoveSpeed"] = mMoveSpeed;

	{
		nlohmann::json cameraJson;

		if (mCamera.ToJson(cameraJson, &GetEntity().GetWorld())) {
			j["Camera"] = cameraJson;
		}
	}

	return j;
}

bool Player::FromJson(const nlohmann::json& j)
{
	mTestMember = j["TestMember"];
	
	if (j.find("MoveSpeed") != j.end()) {
		mMoveSpeed = j["MoveSpeed"];
	}

	if (j.find("Camera") != j.end()) {
		mCamera.FromJson(j["Camera"], &GetEntity().GetWorld());
	}
	
	return true;
}

bool Player::VerifyJson(const nlohmann::json& j) 
{
	if (j.find("TestMember") == j.end()) {
		return false;
	}

	if (!j["TestMember"].is_boolean()) {
		return false;
	}

	return true;
}

void Player::RenderCurrentWeapon(sf::RenderTarget& target) const {
	if (mCurrentWeapon) {
		mCurrentWeapon->Render(target);
	}
}