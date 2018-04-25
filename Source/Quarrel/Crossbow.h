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

#include "CrossbowBolt.h"
#include "Utils.h"

class Crossbow : public Weapon {
public:
	Crossbow(Player& player);

	void OnStep(const qvr::RawInputDevices& inputDevices, const float deltaSeconds) override;
	void Render(sf::RenderTarget& target) override;

private:
	void UpdateOffset(const float deltaSeconds);

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

	struct QuarrelTypeInfo
	{
		sf::Color colour;
		CrossbowBoltEffect effect;
	};

	static const int MaxEquippedQuarrelTypes = 3;

	using QuarrelSlot = std::experimental::optional<QuarrelTypeInfo>;

	std::array<QuarrelSlot, MaxEquippedQuarrelTypes> quarrelSlots;

	struct Quarrel
	{
		QuarrelTypeInfo mTypeInfo;
	};

	std::experimental::optional<Quarrel> mLoadedQuarrel;

	void LoadQuarrel(const QuarrelTypeInfo& quarrel);
	void UnloadQuarrel();

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

	static const sf::Vector2f NoOffset; 
	static const float LoweredOffsetY;
	static const float SecondsToRaise;
	static const float SecondsToLower;

	sf::Vector2f mOffset = sf::Vector2f(0.0f, LoweredOffsetY);
	
	TimeLerper<float> mOffsetYLerper =
		TimeLerper<float>(mOffset.y, NoOffset.y, SecondsToRaise);

	DeltaRadians deltaRadians;
};
