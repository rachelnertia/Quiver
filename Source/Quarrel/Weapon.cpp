#include "Weapon.h"

#include <iostream>

#include <Box2D/Collision/Shapes/b2PolygonShape.h>
#include <Box2D/Dynamics/b2Body.h>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Sprite.hpp>

#include "Quiver/World/World.h"
#include "Quiver/Entity/Entity.h"
#include "Quiver/Entity/CustomComponent/CustomComponent.h"
#include "Quiver/Entity/PhysicsComponent/PhysicsComponent.h"
#include "Quiver/Entity/RenderComponent/RenderComponent.h"
#include "Quiver/Animation/AnimationData.h"
#include "Quiver/Misc/JsonHelpers.h"
#include "Quiver/Input/BinaryInput.h"
#include "Quiver/Input/Mouse.h"
#include "Quiver/Input/Keyboard.h"
#include "Quiver/Input/RawInput.h"

#include "Player.h"

using namespace qvr;

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

Crossbow::Crossbow(Player& player) 
	: Weapon(player) 
{
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
		if (const auto animData = qvr::AnimationData::FromJsonFile(filename))
		{
			mProjectileAnimData = *animData;
			mProjectileAnimId = GenerateAnimationId(mProjectileAnimData);
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

void Crossbow::OnStep(const qvr::RawInputDevices& inputDevices, const float deltaSeconds)
{
	namespace xb = qvr::Xbox360Controller;

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

void Crossbow::Render(sf::RenderTarget& target)
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

void Crossbow::LoadQuarrel(const QuarrelType type)
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

void Crossbow::Shoot()
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

Entity* Crossbow::MakeProjectile(
	World& world,
	const b2Vec2& position,
	const b2Vec2& aimDir,
	const float speed,
	const b2Vec2& inheritedVelocity,
	const sf::Color& color)
{
	b2PolygonShape shape;
	{
		b2Vec2 points[] = { { 0.0f, 0.25f },{ 0.125f, -0.125f },{ -0.125f, -0.125f } };
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