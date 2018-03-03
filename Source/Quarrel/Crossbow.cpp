#include "Crossbow.h"

#include <iostream>
#include <chrono>

#include <Box2D/Collision/Shapes/b2PolygonShape.h>
#include <Box2D/Dynamics/b2Body.h>
#include <Box2D/Dynamics/b2Fixture.h>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Sprite.hpp>

#include <Quiver/World/World.h>
#include <Quiver/Entity/Entity.h>
#include <Quiver/Entity/CustomComponent/CustomComponent.h>
#include <Quiver/Entity/PhysicsComponent/PhysicsComponent.h>
#include <Quiver/Entity/RenderComponent/RenderComponent.h>
#include <Quiver/Animation/AnimationData.h>
#include <Quiver/Misc/JsonHelpers.h>
#include <Quiver/Misc/Logging.h>
#include <Quiver/Misc/Verify.h>
#include <Quiver/Input/BinaryInput.h>
#include <Quiver/Input/Mouse.h>
#include <Quiver/Input/Keyboard.h>
#include <Quiver/Input/RawInput.h>

#include "CrossbowBolt.h"
#include "Player.h"
#include "Utils.h"

using namespace qvr;

Crossbow::Crossbow(Player& player) 
	: Weapon(player) 
	, deltaRadians(player.mCamera.GetRotation())
{
	QuarrelTypeInfo type;
	type.colour = sf::Color::Black;
	type.effect.immediateDamage = 10.0f;

	quarrelSlots[0] = type;

	type.colour = sf::Color::Red;
	type.effect.immediateDamage = 1.0f;
	type.effect.appliesEffect = ActiveEffectType::Burning;

	quarrelSlots[1] = type;

	type.colour = sf::Color::Green;
	type.effect.appliesEffect = ActiveEffectType::Poisoned;

	quarrelSlots[2] = type;

	auto log = GetConsoleLogger();

	auto LogLoadFail = [&log](const char* filename)
	{
		log->error("Crossbow constructor: Couldn't load {},", filename);
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
			log->warn(
				"Crossbow ctor: Didn't manage to get anything useful from {}."
				"Using default RenderComponent JSON for the projectile",
				filename);

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

namespace {

std::function<float(const float)> GetEasingFunctionForCrossbow(const bool raising)
{
	if (raising) {
		auto easeOutBack = [](const float t) {
			const float magnitude = 1.70158f;
			const float scaledTime = (t / 1.0f) - 1.0f;
			return
				(scaledTime * scaledTime * ((magnitude + 1) * scaledTime + magnitude))
				+ 1;
		};
		return easeOutBack;
	}

	auto easeInBack = [](const float t) {
		const float magnitude = 1.70158f;
		const float scaledTime = (t / 1.0f);
		return scaledTime * scaledTime* ((magnitude + 1) * scaledTime - magnitude);
	};

	return easeInBack;
}

}

const float Crossbow::LoweredOffsetY = 1.0f;
const sf::Vector2f Crossbow::NoOffset(0.0f, 0.0f);
const float Crossbow::SecondsToRaise = 0.25f;
const float Crossbow::SecondsToLower = SecondsToRaise;

using namespace std::chrono_literals;

void Crossbow::UpdateOffset(const float deltaSeconds)
{
	// Y offset
	// Account for raising and lowering when drawing the weapon or putting it away.
	float offsetYTarget =
		mOffsetYLerper.Update(
			deltaSeconds,
			GetEasingFunctionForCrossbow(mRaisedState == RaisedState::Raised));

	// Account for forwards/backwards movement.
	const float forwardsVelocity = b2Dot(
		mPlayer.GetEntity().GetPhysics()->GetBody().GetLinearVelocity(),
		mPlayer.mCamera.GetForwards());

	offsetYTarget += 0.025f * std::max(-1.0f, std::min(1.0f, forwardsVelocity));
	
	mOffset.y = Lerp(mOffset.y, offsetYTarget, 0.4f);

	// X offset
	// Account for rotation.

	auto radiansToOffset = [](const float radians) {
		return radians / (b2_pi * 0.5f);
	};

	float offsetXTarget = -radiansToOffset(deltaRadians.Get());

	// Account for sideways movement.
	const float lateralVelocity = b2Dot(
		mPlayer.GetEntity().GetPhysics()->GetBody().GetLinearVelocity(),
		mPlayer.mCamera.GetRightwards());

	offsetXTarget += 0.05f * std::max(-1.0f, std::min(1.0f, lateralVelocity));

	mOffset.x = Lerp(mOffset.x, offsetXTarget, 0.25f);
}

void Crossbow::OnStep(const qvr::RawInputDevices& inputDevices, const float deltaSeconds)
{
	deltaRadians.Update(mPlayer.mCamera.GetRotation());

	namespace xb = qvr::Xbox360Controller;

	std::array<BinaryInput, 2> toggleWeaponInputs = {
		qvr::KeyboardKey::Tab,
		qvr::JoystickAxisThreshold(xb::ToJoystickAxis(xb::Axis::DPad_Vertical), 90.0f, true)
	};

	if (AnyJustActive(inputDevices, toggleWeaponInputs))
	{
		if (mRaisedState == RaisedState::Lowered) {
			mRaisedState = RaisedState::Raised;
			mOffsetYLerper.SetTarget(mOffset.y, NoOffset.y, SecondsToRaise);
		}
		else if (mRaisedState == RaisedState::Raised) {
			mRaisedState = RaisedState::Lowered;
			mOffsetYLerper.SetTarget(mOffset.y, LoweredOffsetY, SecondsToLower);
		}
	}

	UpdateOffset(deltaSeconds);

	if (mRaisedState == RaisedState::Lowered) return;

	switch (mCockedState) {
	case CockedState::Uncocked:
		if (AnyJustActive(inputDevices, mCockInputs)) {
			mCockedState = CockedState::Cocked;
		}
		break;
	case CockedState::Cocked:
		if (!mLoadedQuarrel)
		{
			BinaryInput loadInputs1[] = { qvr::KeyboardKey::Num1, qvr::JoystickButton(xb::Button::B) };
			BinaryInput loadInputs2[] = { qvr::KeyboardKey::Num2, qvr::JoystickButton(xb::Button::X) };
			BinaryInput loadInputs3[] = { qvr::KeyboardKey::Num3, qvr::JoystickButton(xb::Button::Y) };

			if (quarrelSlots[0].has_value() &&
				AnyActive(inputDevices, loadInputs1)) 
			{
				LoadQuarrel(*quarrelSlots[0]);
				break;
			}
			else if (
				quarrelSlots[1].has_value() &&
				AnyActive(inputDevices, loadInputs2))
			{
				LoadQuarrel(*quarrelSlots[1]);
				break;
			}
			else if (
				quarrelSlots[2].has_value() &&
				AnyActive(inputDevices, loadInputs3))
			{
				LoadQuarrel(*quarrelSlots[2]);
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

			mCockedState = CockedState::Uncocked;
		}

		break;
	}
}

void Crossbow::Render(sf::RenderTarget& target)
{
	sf::Vector2f targetSize((float)target.getSize().x, (float)target.getSize().y);

	sf::Sprite crossbowSprite;

	crossbowSprite.setTexture(mTexture);

	auto GetTextureRectFromState = [](const CockedState state, const sf::Vector2u& textureSize)
	{
		const int frameIndex = [state]()
		{
			switch (state)
			{
			case CockedState::Cocked:
				return 1;
			}
			return 0;
		}();
		return sf::IntRect(
			sf::Vector2i(0, frameIndex * textureSize.y / 2),
			sf::Vector2i(textureSize.x, textureSize.y / 2));
	};

	crossbowSprite.setTextureRect(GetTextureRectFromState(mCockedState, mTexture.getSize()));

	// 25% of the crossbow is below the bottom of the screen.
	const float hidden = 0.25f;
	const float crossbowHeight = (float)crossbowSprite.getTextureRect().height;

	crossbowSprite.setOrigin(
		crossbowSprite.getTextureRect().width / 2.0f,
		crossbowHeight - (crossbowHeight * hidden));

	const sf::Vector2f centre(targetSize.x / 2.0f, (float)targetSize.y);
	const sf::Vector2f offset(
		(targetSize.x * 0.5f) * mOffset.x,
		(targetSize.y * 0.5f) * mOffset.y);
	const sf::Vector2f position = centre + offset;

	crossbowSprite.setPosition(position);

	const float scale = ((float)targetSize.x / (float)crossbowSprite.getTextureRect().width / 2);

	crossbowSprite.setScale(scale, scale);

	const sf::Color darkColor = mPlayer.GetEntity().GetWorld().GetAmbientLight().mColor;

	crossbowSprite.setColor(darkColor);

	target.draw(crossbowSprite);

	// Now draw the crossbow bolt
	if (mCockedState == CockedState::Cocked && mLoadedQuarrel)
	{
		sf::Sprite boltSprite;

		boltSprite.setTexture(mLoadedBoltTexture);

		boltSprite.setOrigin(
			boltSprite.getTextureRect().width / 2.0f,
			crossbowHeight - (crossbowHeight * hidden));

		boltSprite.setScale(scale, scale);

		boltSprite.setPosition(position);

		boltSprite.setColor(
			mLoadedQuarrel.value().mTypeInfo.colour * darkColor);

		target.draw(boltSprite);
	}
}

void Crossbow::LoadQuarrel(const QuarrelTypeInfo& type)
{
	if (!qvrVerify(mCockedState == CockedState::Cocked)) return;
	if (!qvrVerify(!mLoadedQuarrel)) return;

	mLoadedQuarrel = Quarrel{ type };
}

using json = nlohmann::json;

Entity* MakeCrossbowBolt(
	World& world,
	const b2Vec2& position,
	const b2Vec2& aimDir,
	const float speed,
	const b2Vec2& inheritedVelocity,
	const json& renderCompJson,
	const sf::Color& color,
	const CrossbowBoltEffect& effect,
	const qvr::AnimationId animation,
	const qvr::AnimationData& animationData)
{
	b2PolygonShape shape;
	{
		b2Vec2 points[] = { { 0.0f, 0.25f },{ 0.125f, -0.125f },{ -0.125f, -0.125f } };
		shape.Set(points, 3);
	}

	Entity* projectile = world.CreateEntity(
		shape,
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

		projRenderComp->FromJson(renderCompJson);

		projRenderComp->SetColor(color);

		// Load the animation into the AnimatorCollection if it hasn't already been done.
		if (!world.GetAnimators().GetAnimations().Contains(animation))
		{
			world.GetAnimators().AddAnimation(animationData);
		}

		projRenderComp->SetAnimation(animation);
	}

	// Set up CustomComponent
	{
		auto bolt = std::make_unique<CrossbowBolt>(*projectile);
		bolt->effect = effect;
		projectile->AddCustomComponent(std::move(bolt));
	}

	return projectile;
}

void Crossbow::Shoot()
{
	if (!qvrVerify(mLoadedQuarrel))
	{
		return;
	}

	mShootSound.setBuffer(mShootSoundBuffer);
	mShootSound.setRelativeToListener(true);
	mShootSound.play();

	const auto position = mPlayer.GetEntity().GetPhysics()->GetPosition();
	const auto direction = mPlayer.mCamera.GetForwards();
	const auto velocity = mPlayer.GetEntity().GetPhysics()->GetBody().GetLinearVelocity();

	MakeCrossbowBolt(
		mPlayer.GetEntity().GetWorld(),
		position,
		direction,
		40.0f,
		0.1f * velocity,
		mProjectileRenderCompJson,
		mLoadedQuarrel->mTypeInfo.colour,
		mLoadedQuarrel->mTypeInfo.effect,
		mProjectileAnimId,
		mProjectileAnimData);

	mLoadedQuarrel.reset();

	mCockedState = CockedState::Uncocked;
}
