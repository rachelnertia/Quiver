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
	void UpdateOffset(const float deltaSeconds);

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

	static const sf::Vector2f NoOffset; 
	static const float LoweredOffsetY;
	static const float SecondsToRaise;
	static const float SecondsToLower;

	sf::Vector2f mOffset = sf::Vector2f(0.0f, LoweredOffsetY);
	
	TimeLerper<float> mOffsetYLerper =
		TimeLerper<float>(mOffset.y, NoOffset.y, SecondsToRaise);

	class DeltaRadians
	{
		float last = 0.0f;
		float delta = 0.0f;
	public:
		void Update(const float currentRadians) {
			const float pi = b2_pi;
			const float tau = 2.0f * pi;
			
			float val = currentRadians - last;
			last = currentRadians;
			
			// Account for when the current and last are on different sides of the pole.
			val += (val > pi) ? -tau : (val < -pi) ? tau : 0.0f;
			

			// Limit the angle.
			const float max = pi * 0.25f;
			delta = std::max(-max, std::min(val, max));
		}

		float Get() const {
			return delta;
		}
	};

	DeltaRadians deltaRadians;
};
