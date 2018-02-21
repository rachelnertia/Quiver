#pragma once

#include "Weapon.h"

#include <chrono>
#include <vector>

#include <optional.hpp>
#include <SFML/Audio/Sound.hpp>
#include <SFML/Audio/SoundBuffer.hpp>
#include <SFML/Graphics/Texture.hpp>

#include "Quiver/Animation/AnimationData.h"
#include "Quiver/Input/BinaryInput.h"
#include "Quiver/Input/Mouse.h"
#include "Quiver/Input/Keyboard.h"
#include "Quiver/Input/RawInput.h"

#include "Utils.h"

class Crossbow : public Weapon {
public:
	Crossbow(Player& player);

	void OnStep(const qvr::RawInputDevices& inputDevices, const float deltaSeconds) override;
	void Render(sf::RenderTarget& target) override;

private:
	qvr::Entity* MakeProjectile(
		qvr::World& world,
		const b2Vec2& position,
		const b2Vec2& aimDir,
		const float speed,
		const b2Vec2& inheritedVelocity,
		const sf::Color& color);

	enum class RaisedState {
		Lowered,
		Raised
	};

	RaisedState mRaisedState = RaisedState::Raised;

	enum class CockedState {
		Uncocked = 0,
		Cocked
	};

	CockedState mCockedState = CockedState::Uncocked;

	sf::Texture mTexture;
	sf::Texture mLoadedBoltTexture;

	sf::SoundBuffer mShootSoundBuffer;
	sf::Sound mShootSound;

	qvr::AnimationData mProjectileAnimData;
	qvr::AnimationId   mProjectileAnimId = qvr::AnimationId::Invalid;

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

	void LoadQuarrel(const QuarrelType type);

	void Shoot();

	std::vector<BinaryInput> mCockInputs =
	{
		qvr::KeyboardKey::R,
		qvr::JoystickButton(qvr::Xbox360Controller::Button::A)
	};

	std::vector<BinaryInput> mTriggerInputs =
	{
		qvr::MouseButton::Left,
		qvr::KeyboardKey::Space,
		qvr::JoystickAxisThreshold(qvr::Xbox360Controller::Axis::Trigger_Right, -50.0f, false)
	};

	sf::Vector2f mOffset = sf::Vector2f(0.0f, 1.0f);
	TimeLerper<sf::Vector2f> mOffsetLerper =
		TimeLerper<sf::Vector2f>(mOffset, sf::Vector2f(0.0f, 0.0f), 0.25f);
};
