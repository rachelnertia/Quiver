#include "Player.h"

#include <array>
#include <fstream>
#include <iostream>

#include <Box2D/Common/b2Math.h>
#include <Box2D/Dynamics/b2Body.h>
#include <Box2D/Dynamics/b2Fixture.h>
#include <Box2D/Dynamics/b2World.h>
#include <Box2D/Collision/Shapes/b2CircleShape.h>
#include <Box2D/Collision/Shapes/b2PolygonShape.h>
#include <ImGui/imgui.h>
#include <SFML/Audio.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>

#include "Quiver/Audio/Listener.h"
#include "Quiver/Animation/Animators.h"
#include "Quiver/Animation/AnimationData.h"
#include "Quiver/Entity/Entity.h"
#include "Quiver/Entity/RenderComponent/RenderComponent.h"
#include "Quiver/Entity/PhysicsComponent/PhysicsComponent.h"
#include "Quiver/Input/BinaryInput.h"
#include "Quiver/Input/Mouse.h"
#include "Quiver/Input/Keyboard.h"
#include "Quiver/Input/RawInput.h"
#include "Quiver/Misc/JsonHelpers.h"
#include "Quiver/Misc/Logging.h"
#include "Quiver/World/World.h"

#include "Crossbow.h"
#include "PlayerInput.h"
#include "Utils.h"

using namespace std::chrono_literals;
using namespace qvr;

namespace xb = qvr::Xbox360Controller;

void AddFilterCategories(b2Fixture& fixture, const int16 categories) {
	b2Filter filter = fixture.GetFilterData();
	filter.categoryBits |= categories;
	fixture.SetFilterData(filter);
}

Player::Player(Entity& entity)
	: Player(
		entity, 
		CameraOwner(entity.GetWorld(), entity.GetPhysics()->GetBody().GetTransform()),
		PlayerDesc())
{}

Player::Player(Entity& entity, CameraOwner&& camera, const PlayerDesc& desc)
	: CustomComponent(entity)
	, mCurrentWeapon(std::make_unique<Crossbow>(*this))
	, cameraOwner(std::move(camera))
	, mMoveSpeed(desc.moveSpeed)
	, mDamage(desc.damage)
	, m_ActiveEffects(desc.activeEffects)
	, fovLerper(GetCamera().GetFovRadians(), b2_pi / 2, 0.1f)
{
	AddFilterCategories(
		*GetEntity().GetPhysics()->GetBody().GetFixtureList(),
		FixtureFilterCategories::Player);

	GetCamera().SetOverlayDrawer([this](sf::RenderTarget& target) {
		this->RenderCurrentWeapon(target);
		this->RenderActiveEffects(target);
		this->RenderHud(target);
	});
}

class DeadPlayer : public CustomComponent
{
public:
	DeadPlayer(Entity& entity, Player& player)
		: CustomComponent(entity)
		, mCamera(std::move(player.cameraOwner))
	{
		mCamera.camera.SetHeight(0.0f);
	}

	std::string GetTypeName() const override { return "DeadPlayer"; }

	void OnStep(const std::chrono::duration<float> deltaTime) override {
		b2Body& body = GetEntity().GetPhysics()->GetBody();

		mCamera.camera.SetPosition(body.GetPosition());
		mCamera.camera.SetRotation(body.GetAngle());

		qvr::UpdateListener(mCamera.camera);
	}

private:
	CameraOwner mCamera;

};

namespace {

void Toggle(bool& b) {
	b = !b;
}

bool EnemyAhead(
	const b2World& world,
	const b2Vec2& startPos,
	const b2Vec2& direction) 
{
	class ClosestHitCallback : public b2RayCastCallback
	{
	public:
		b2Fixture* closestFixture = nullptr;

		float32 ReportFixture(
			b2Fixture* fixture,
			const b2Vec2& point,
			const b2Vec2& normal,
			float32 fraction) override
		{
			using namespace FixtureFilterCategories;
			const unsigned ignoreMask = RenderOnly | Projectile | Fire | Sensor;

			if ((GetCategoryBits(*fixture) & ignoreMask) != 0)
			{
				return -1;
			}

			closestFixture = fixture;

			return fraction;
		}
	};
	
	ClosestHitCallback cb;

	const float range = 20.0f;

	world.RayCast(&cb, startPos, startPos + (range * direction));

	if (cb.closestFixture) {
		const uint16 categoryBits = GetCategoryBits(*cb.closestFixture);
		const auto result = (categoryBits & FixtureFilterCategories::Enemy);
		if (result != 0)
		{
			return true;
		}
	}

	return false;
}

}

void Player::HandleInput(
	qvr::RawInputDevices& devices, 
	const std::chrono::duration<float> deltaTime)
{
	using namespace Quarrel;
	
	b2Body& body = GetEntity().GetPhysics()->GetBody();

	// Move. Don't check player input if the player is being pushed quickly by something
	// already.
	if (mMoveSpeed > body.GetLinearVelocity().Length()) {
		const b2Vec2 dir = GetDirectionVector(devices);

		// Don't override the body's velocity if the player isn't providing any input.
		if (dir.LengthSquared() != 0.0f) {
			// Rotate the direction vector into the body's frame.
			b2Vec2 moveDir = b2Mul(b2Rot(body.GetAngle()), dir);
			body.SetLinearVelocity(mMoveSpeed * moveDir);
		}
	}

	// Turn.
	{
		float rotateAngle = GetTurnAngle(devices);
		
		const bool stickyLook =
			EnemyAhead(
				*GetEntity().GetWorld().GetPhysicsWorld(),
				body.GetPosition(),
				GetCamera().GetForwards());

		if (stickyLook) {
			const float stickyLookModifier = 0.5f;
			rotateAngle *= stickyLookModifier;
		}

		if (rotateAngle != 0.0f) {
			const float rotateSpeed = 3.14f; // radians per second

			const float rotation = 
				rotateAngle * 
				rotateSpeed * 
				deltaTime.count();

			body.SetTransform(body.GetPosition(), body.GetAngle() + rotation);
		}
	}

	// Debug stuff!
	{
		const int debugDamage = (int)std::ceil(20.0f * deltaTime.count());

		if (devices.GetKeyboard().IsDown(qvr::KeyboardKey::U)) {
			AddDamage(mDamage, debugDamage);
		}

		if (devices.GetKeyboard().IsDown(qvr::KeyboardKey::J)) {
			RemoveDamage(mDamage, debugDamage);
		}

		if (devices.GetKeyboard().JustDown(qvr::KeyboardKey::K)) {
			Toggle(mCannotDie);
		}
	}

	if (mCurrentWeapon != nullptr) {
		mCurrentWeapon->HandleInput(devices, deltaTime.count());
	}
}

namespace {

const int PlayerDamageMax = 100;

const int EnemyProjectileDamage = 20;

const int EnemyAttackDamage = 30;

}

void Player::OnStep(const std::chrono::duration<float> deltaTime)
{
	auto log = GetConsoleLogger();
	const char* logCtx = "Player::OnStep:";

	ApplyFires(m_FiresInContact, m_ActiveEffects);
	
	for (auto& effect : m_ActiveEffects.container) {
		if (UpdateEffect(effect, deltaTime)) {
			ApplyEffect(effect, mDamage);
		}
		ApplyEffect(effect, *GetEntity().GetGraphics());
	}

	RemoveExpiredEffects(m_ActiveEffects);

	if (mCannotDie == false &&
		HasExceededLimit(mDamage)) 
	{
		log->debug("{} Oh no! I've taken too much damage!", logCtx);
		
		GetEntity().AddCustomComponent(std::make_unique<DeadPlayer>(GetEntity(), *this));

		// Super important to return here!
		return;
	}

	b2Body& body = GetEntity().GetPhysics()->GetBody();

	GetCamera().SetPosition(body.GetPosition());
	GetCamera().SetRotation(body.GetAngle());

	GetCamera().SetFov(fovLerper.Update(deltaTime.count()));

	qvr::UpdateListener(GetCamera());
}

void Player::OnBeginContact(Entity& other, b2Fixture& myFixture, b2Fixture& otherFixture)
{
	auto log = GetConsoleLogger();
	const char* logCtx = "Player::OnBeginContact";

	if (FlagsAreSet(
		FixtureFilterCategories::EnemyAttack,
		otherFixture.GetFilterData().categoryBits))
	{
		log->debug("{} Player touching EnemyAttack fixture", logCtx);

		AddDamage(mDamage, EnemyAttackDamage);

		b2Body& myBody = GetEntity().GetPhysics()->GetBody();
		const b2Vec2 impulseDirection = [&myBody, &otherFixture]() {
			b2Vec2 direction = myBody.GetPosition() - otherFixture.GetBody()->GetPosition();
			direction.Normalize();
			return direction;
		}();

		myBody.ApplyLinearImpulse(5.0f * impulseDirection, myBody.GetPosition(), true);
	}
	
	if (other.GetCustomComponent()) {

		log->debug(
			"{} Player beginning contact with {}...", 
			logCtx, 
			other.GetCustomComponent()->GetTypeName());

		if (other.GetCustomComponent()->GetTypeName() == "EnemyProjectile") 
		{
			log->debug("{} Player taking damage", logCtx);

			AddDamage(mDamage, EnemyProjectileDamage);
		}
	}

	::OnBeginContact(m_FiresInContact, otherFixture);
}

void Player::OnEndContact(Entity& other, b2Fixture& myFixture, b2Fixture& otherFixture)
{
	auto log = GetConsoleLogger();

	if (other.GetCustomComponent()) {
		log->debug("Player finishing contact with {}...", other.GetCustomComponent()->GetTypeName());
	}

	::OnEndContact(m_FiresInContact, otherFixture);
}

class PlayerEditor : public CustomComponentEditorType<Player>
{
public:
	PlayerEditor(Player& player) : CustomComponentEditorType(player) {}
	
	void GuiControls() override {
		ImGui::SliderFloat("Move Speed", &Target().mMoveSpeed, 0.0f, 20.0f);
	}
};

std::unique_ptr<CustomComponentEditor> Player::CreateEditor() {
	return std::make_unique<PlayerEditor>(*this);
}

nlohmann::json Player::ToJson() const
{
	nlohmann::json j;

	j["MoveSpeed"] = mMoveSpeed;

	{
		nlohmann::json cameraJson;

		if (GetCamera().ToJson(cameraJson, &GetEntity().GetWorld())) {
			j["Camera"] = cameraJson;
		}
	}

	return j;
}

bool Player::FromJson(const nlohmann::json& j)
{
	if (j.find("MoveSpeed") != j.end()) {
		mMoveSpeed = j["MoveSpeed"];
	}

	if (j.find("Camera") != j.end()) {
		GetCamera().FromJson(j["Camera"], &GetEntity().GetWorld());
	}
	
	return true;
}

PlayerDesc Player::GetDesc() {
	return PlayerDesc{
		m_ActiveEffects,
		mDamage,
		mMoveSpeed
	};
}

void Player::RenderCurrentWeapon(sf::RenderTarget& target) const {
	if (mCurrentWeapon) {
		mCurrentWeapon->Render(target);
	}
}

void DrawDamage(sf::RenderTarget& target, const DamageCount& counter)
{
	sf::RectangleShape background;
	const float scale = 0.8f;
	background.setSize(
		scale * sf::Vector2f(
			target.getSize().x / 5.0f,
			target.getSize().y / 12.0f));
	background.setOrigin(background.getSize());
	const float indent = 10.0f;
	background.setPosition(
		target.getSize().x - indent,
		target.getSize().y - indent);
	background.setFillColor(sf::Color(0, 0, 0, 128));
	background.setOutlineColor(sf::Color::Black);
	background.setOutlineThickness(5.0f);
	target.draw(background);

	sf::RectangleShape bar;
	bar.setFillColor(sf::Color(255, 0, 128));
	bar.setOutlineColor(sf::Color(255, 0, 0));
	bar.setOutlineThickness(-3.0f);
	bar.setSize(
		sf::Vector2f(
			background.getSize().x * ((float)counter.damage / counter.max),
			background.getSize().y));
	bar.setOrigin(bar.getSize());
	bar.setPosition(background.getPosition());
	target.draw(bar);
}

void Player::RenderHud(sf::RenderTarget& target) const {
	if (HasTakenDamage(mDamage)) {
		DrawDamage(target, mDamage);
	}
}

namespace {

void RenderActiveEffects(
	const gsl::span<const ActiveEffect> effects,
	sf::RenderTarget& target)
{
	if (effects.empty()) return;

	auto it = std::find_if(
		std::begin(effects), 
		std::end(effects), 
		[](const ActiveEffect& effect)
		{
			return effect.type == ActiveEffectType::Burning;
		});

	if (it == std::end(effects)) return;

	sf::RectangleShape rectShape;
	rectShape.setSize(sf::Vector2f((float)target.getSize().x, (float)target.getSize().y));
	rectShape.setFillColor(sf::Color(255, 0, 0, 128));
	target.draw(rectShape);
}

}

void Player::RenderActiveEffects(sf::RenderTarget& target) const {
	::RenderActiveEffects(m_ActiveEffects.container, target);
}
