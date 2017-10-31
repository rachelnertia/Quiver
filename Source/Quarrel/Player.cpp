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

class Weapon {
protected:
	Player& mPlayer;
public:
	Weapon(Player& player) : mPlayer(player) {};
	
	virtual void OnStep(
		const qvr::RawInputDevices& inputDevices,
		const float deltaSeconds) = 0;

	virtual void Render(sf::RenderTarget& target) {}
};

class CrossbowBolt : public CustomComponent
{
public:
	CrossbowBolt(Entity& entity) : CustomComponent(entity) {}

	void OnBeginContact(Entity& other) override
	{
		this->SetRemoveFlag(true);
	}
	
	std::string GetTypeName() const override { return "CrossbowBolt"; }

};

class Crossbow : public Weapon {
private:
	Entity* MakeProjectile(
		World& world, 
		const b2Vec2& position, 
		const b2Vec2& aimDir, 
		const float speed, 
		const b2Vec2& inheritedVelocity,
		const sf::Color& color)
	{
		b2PolygonShape shape;
		{
			b2Vec2 points[] = { { 0.0f, 0.25f }, { 0.125f, -0.125f }, { -0.125f, -0.125f} };
			shape.Set(points, 3);
		}

		Entity* projectile = world.CreateEntity(shape, 
			position + aimDir, 
			b2Atan2(aimDir.y, aimDir.x) - (b2_pi * 0.5f));

		if (projectile == nullptr) {
			return nullptr;
		}

		// Set up body.
		{
			b2Body& projBody = projectile->GetPhysics()->GetBody();
			projBody.SetType(b2_dynamicBody);
			projBody.SetLinearVelocity((speed * aimDir) + inheritedVelocity);
			projBody.SetBullet(true);
		}

		// Set up RenderComponent.
		{
			projectile->AddGraphics();
			RenderComponent* projRenderComp = projectile->GetGraphics();
			
			projRenderComp->FromJson(mProjectileRenderCompJson);

			projRenderComp->SetColor(color);

			// Load the animation into the AnimationSystem if it hasn't already been done.
			if (!world.GetAnimationSystem().AnimationExists(mProjectileAnimId))
			{
				world.GetAnimationSystem().AddAnimation(mProjectileAnimData);
			}

			projRenderComp->SetAnimation(mProjectileAnimId);
		}

		// Set up CustomComponent
		{
			projectile->SetInput(std::make_unique<CrossbowBolt>(*projectile));
		}

		return projectile;
	}

	enum class State {
		Uncocked = 0,
		Cocked
	};

	State mState = State::Uncocked;

	sf::Texture mTexture;
	sf::Texture mLoadedBoltTexture;

	sf::SoundBuffer mShootSoundBuffer;
	sf::Sound mShootSound;

	AnimationData mProjectileAnimData;
	AnimationId   mProjectileAnimId = AnimationId::Invalid;

	nlohmann::json mProjectileRenderCompJson;

	enum class QuarrelType
	{
		Black = 0, Red, Blue, Count
	};

	struct QuarrelTypeInfo
	{
		sf::Color colour;
	};

	const std::array<QuarrelTypeInfo, (size_t)QuarrelType::Count> quarrelTypes =
	{
		sf::Color::Black, sf::Color::Red, sf::Color::Blue
	};

	struct Quarrel
	{
		QuarrelType     mType;
		QuarrelTypeInfo mTypeInfo;
	};

	std::experimental::optional<Quarrel> mLoadedQuarrel;

	void LoadQuarrel(const QuarrelType type)
	{
		// TODO: We can do away with the Loaded State, actually.
		assert(mState == State::Cocked);
		assert(!mLoadedQuarrel);
		if (mState != State::Cocked || mLoadedQuarrel)
		{
			return;
		}

		mLoadedQuarrel = Quarrel{ type, quarrelTypes[(int)type] };
	}

	void Shoot()
	{
		assert(mLoadedQuarrel);
		if (!mLoadedQuarrel)
		{
			return;
		}

		mShootSound.setBuffer(mShootSoundBuffer);
		mShootSound.setRelativeToListener(true);
		mShootSound.play();

		const auto position = mPlayer.GetEntity().GetPhysics()->GetPosition();
		const auto direction = mPlayer.mCamera.GetForwards();
		const auto velocity = mPlayer.GetEntity().GetPhysics()->GetBody().GetLinearVelocity();

		MakeProjectile(
			mPlayer.GetEntity().GetWorld(),
			position,
			direction,
			10.0f,
			0.5f * velocity,
			mLoadedQuarrel.value().mTypeInfo.colour);

		mLoadedQuarrel.reset();

		mState = State::Uncocked;
	}
	
public:
	Crossbow(Player& player) : Weapon(player) {
		auto LogLoadFail = [](const char* filename)
		{
			std::cout << "Crossbow ctor: Couldn't load " << filename << "\n";
		};

		// Load crossbow textures.
		{
			const char* filename = "Textures/crossbow.png";
			if (!mTexture.loadFromFile(filename)) {
				LogLoadFail(filename);
			}
		}
		{
			const char* filename = "Textures/loaded_crossbow_bolt.png";
			if (!mLoadedBoltTexture.loadFromFile(filename)) {
				LogLoadFail(filename);
			}
		}

		// Load shoot sound.
		{
			const char* filename = "Audio/crossbow_shoot.wav";
			if (!mShootSoundBuffer.loadFromFile(filename)) {
				LogLoadFail(filename);
			}
		}

		// Load Animation for projectile.
		{
			const char* filename = "Animations/rotating_thing.json";
			if (const auto animData = AnimationData::FromJsonFile(filename))
			{
				mProjectileAnimData = *animData;
				mProjectileAnimId   = GenerateAnimationId(mProjectileAnimData);
			}
			else
			{
				LogLoadFail(filename);
			}
		}

		// Load JSON for projectile.
		{
			const char* filename = "projectile.json";
			
			mProjectileRenderCompJson = JsonHelp::LoadJsonFromFile(filename);

			if (mProjectileRenderCompJson.empty())
			{
				std::cout << "Crossbow ctor: Didn't manage to get anything useful from " << filename << 
					". Using default RenderComponent JSON for the projectile \n";

				mProjectileRenderCompJson =
				{
					{ "RenderType", "Sprite" },
					{ "Texture", "textures/rotating_thing.png" },
					//{ "Animation", { { "File", "animations/rotating_thing.json" } } }
					{ "SpriteRadius", 0.2f }
				};
			}
		}
	}

private:
	std::vector<BinaryInput> mCockInputs =
	{
		qvr::KeyboardKey::R, 
		qvr::JoystickButton(xb::Button::A)
	};

	std::vector<BinaryInput> mTriggerInputs =
	{
		qvr::MouseButton::Left,
		qvr::KeyboardKey::Space,
		qvr::JoystickAxisThreshold(xb::Axis::Trigger_Right, -50.0f, false)
	};

public:
	void OnStep(const qvr::RawInputDevices& inputDevices, const float deltaSeconds) override
	{
		switch (mState) {
		case State::Uncocked:
			if (AnyJustActive(inputDevices, mCockInputs)) {
				mState = State::Cocked;
			}
			break;
		case State::Cocked:
			if (!mLoadedQuarrel)
			{
				if (AnyActive(inputDevices, { qvr::KeyboardKey::Num1, qvr::JoystickButton(xb::Button::B) })) {
					LoadQuarrel(QuarrelType::Black);
					break;
				}
				else if (AnyActive(inputDevices, { qvr::KeyboardKey::Num2, qvr::JoystickButton(xb::Button::X) })) {
					LoadQuarrel(QuarrelType::Blue);
					break;
				}
				else if (AnyActive(inputDevices, { qvr::KeyboardKey::Num3, qvr::JoystickButton(xb::Button::Y) })) {
					LoadQuarrel(QuarrelType::Red);
					break;
				}
			}

			if (AnyJustActive(inputDevices, mTriggerInputs))
			{
				if (mLoadedQuarrel)
				{
					Shoot();
					break;
				}

				mState = State::Uncocked;
			}

			break;
		}
	}

	void Render(sf::RenderTarget& target) override 
	{
		sf::Vector2f targetSize((float)target.getSize().x, (float)target.getSize().y);
		
		// TODO: Rename to crossbowSprite.
		sf::Sprite sprite;

		sprite.setTexture(mTexture);

		auto GetTextureRectFromState = [](const State state, const sf::Vector2u& textureSize)
		{
			const int frameIndex = [state]()
			{
				switch (state)
				{
				case State::Cocked:
					return 1;
				}
				return 0;
			}();
			return sf::IntRect(
				sf::Vector2i(0, frameIndex * textureSize.y / 2),
				sf::Vector2i(textureSize.x, textureSize.y / 2));
		};

		sprite.setTextureRect(GetTextureRectFromState(mState, mTexture.getSize()));

		sprite.setOrigin(
			sprite.getTextureRect().width / 2.0f,
			(float)sprite.getTextureRect().height);

		sprite.setPosition(targetSize.x / 2.0f, (float)targetSize.y);

		const float scale = ((float)targetSize.x / (float)sprite.getTextureRect().width / 2);

		sprite.setScale(scale, scale);

		const sf::Color darkColor = mPlayer.GetEntity().GetWorld().GetAmbientLight().mColor;

		sprite.setColor(darkColor);

		target.draw(sprite);

		// Now draw the crossbow bolt
		if (mState == State::Cocked && mLoadedQuarrel)
		{
			sf::Sprite boltSprite;

			boltSprite.setTexture(mLoadedBoltTexture);

			boltSprite.setOrigin(
				boltSprite.getTextureRect().width / 2.0f,
				(float)boltSprite.getTextureRect().height);

			boltSprite.setScale(scale, scale);

			boltSprite.setPosition(targetSize.x / 2.0f, (float)targetSize.y);

			boltSprite.setColor(
				mLoadedQuarrel.value().mTypeInfo.colour * darkColor);

			target.draw(boltSprite);
		}
	}

};

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
	if (other.GetCustomComponent()) {
		std::cout << "Player finishing contact with " << other.GetCustomComponent()->GetTypeName() << "...\n";
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