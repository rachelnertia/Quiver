#include "Camera3D.h"

#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Joystick.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>
#include <SFML/Window/Window.hpp>

#include "Quiver/Input/Xbox360Controller.h"
#include "Quiver/World/World.h"

namespace qvr {

// We have to manually define these constructors 
// because we don't want to copy the overlay-drawer function object.

Camera3D::Camera3D(const Camera3D& other)
	: mTransform(other.mTransform)
	, mHeight(other.mHeight)
	, mBaseHeight(other.mBaseHeight)
	, mFovRadians(other.mFovRadians)
{}

Camera3D& Camera3D::operator=(const Camera3D& other) {
	mTransform = other.mTransform;
	mHeight = other.mHeight;
	mBaseHeight = other.mBaseHeight;
	mFovRadians = other.mFovRadians;
	return *this;
}

bool Camera3D::ToJson(nlohmann::json& j, const World* world) const {
	if (world) {
		if (world->GetMainCamera() == this) {
			j["IsMainCamera"] = true;
		}
	}

	return true;
}

bool Camera3D::FromJson(const nlohmann::json& j, World* world) {
	if (!j.is_object())
		return false;

	if (world) {
		if (j.value<bool>("IsMainCamera", false)) {
			const bool result = world->SetMainCamera(*this);
			assert(result);
		}
	}

	return true;
}

void PitchBy(Camera3D& camera, const float radians) {
	camera.SetPitch(camera.GetPitchRadians() + radians);
}

// TODO: Maybe just pass the mouse delta. The caller would be responsible for positioning 
// the mouse and calculating the delta.
void FreeControl(
	Camera3D& camera,
	const float dt,
	const bool mouselook,
	const sf::Window* windowForMouselook)
{
	namespace xbc = Xbox360Controller;

	{
		const float speed = 1.0f; // speed in metres per second.

		b2Vec2 move(0.0f, 0.0f);

		b2Vec2 forwards = camera.GetForwards();
		b2Vec2 right = b2Vec2(-forwards.y, forwards.x);

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) {
			move += forwards;
		}
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) {
			move -= forwards;
		}

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) {
			move += right;
		}
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) {
			move -= right;
		}

		if (sf::Joystick::isConnected(0)) {
			float x = sf::Joystick::getAxisPosition(0, sf::Joystick::Axis::X);
			float y = -sf::Joystick::getAxisPosition(0, sf::Joystick::Axis::Y);

			x /= 100.0f;
			y /= 100.0f;

			float threshold = 0.25f;

			x = abs(x) > threshold ? x : 0;
			y = abs(y) > threshold ? y : 0;

			move += x * right;
			move += y * forwards;
		}

		move.Normalize();

		camera.MoveBy(speed * dt * move);
	}
	{
		const float rotateSpeed = 3.14f; // radians per second

		float rotationHorizontal = 0.0f, rotationVertical = 0.0f;

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
			rotationHorizontal += rotateSpeed;
		}
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
			rotationHorizontal -= rotateSpeed;
		}
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) {
			rotationVertical += rotateSpeed;
		}
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) {
			rotationVertical -= rotateSpeed;
		}

		auto GetJoystickLook = []() {
			if (!sf::Joystick::isConnected(0)) return sf::Vector2f{ 0.0f, 0.0f };

			const sf::Vector2f raw(
				0, 0);
			//sf::Joystick::getAxisPosition(
			//	0, 
			//	xbc::ToJoystickAxis(xbc::Axis::AnalogStick_Right_Horizontal)),
			//sf::Joystick::getAxisPosition(
			//	0, 
			//	xbc::ToSfmlAxis(xbc::Axis::AnalogStick_Right_Vertical)));

			const int thresholdRaw = 25;

			const sf::Vector2f processed(
				(std::abs(raw.x) > thresholdRaw ? raw.x : 0.0f) / 100,
				(std::abs(raw.y) > thresholdRaw ? raw.y : 0.0f) / 100);

			return processed;
		};

		{
			sf::Vector2f joystickDelta = GetJoystickLook();

			rotationHorizontal += joystickDelta.x * rotateSpeed;
			rotationVertical -= joystickDelta.y * rotateSpeed;
		}

		auto GetMouseDelta = [mouselook, windowForMouselook]()
		{
			if (!mouselook || !windowForMouselook) return sf::Vector2f{ 0.0f, 0.0f };

			sf::Vector2i windowHalfSize =
				sf::Vector2i(
					windowForMouselook->getSize().x / 2,
					windowForMouselook->getSize().y / 2);

			sf::Vector2i currentMousePos = sf::Mouse::getPosition(*windowForMouselook);

			sf::Mouse::setPosition(windowHalfSize, *windowForMouselook);

			return sf::Vector2f(currentMousePos - windowHalfSize);
		};

		{
			sf::Vector2f mouseDelta = GetMouseDelta();

			rotationHorizontal += rotateSpeed * mouseDelta.x;
			rotationVertical -= rotateSpeed * mouseDelta.y;
		}

		// By using the multiple inputs at the same time it's possible to turn
		// at double rotateSpeed, unless we clamp it.
		rotationHorizontal = std::min(std::max(rotationHorizontal, -rotateSpeed), rotateSpeed);
		rotationVertical = std::min(std::max(rotationVertical, -rotateSpeed), rotateSpeed);

		if (rotationHorizontal != 0.0f) {
			rotationHorizontal *= dt;

			camera.RotateBy(rotationHorizontal);

			auto AreEquivalent = [](const float a, const float b, const float error) {
				return (fabsf(a - b) < fabsf(error));
			};
			if (!AreEquivalent(camera.GetForwards().Length(), 1.0f, 0.0001f)) {
				std::cout << "Error: Camera3D.GetForwards().Length() == "
					<< std::fixed << std::setprecision(8) << camera.GetForwards().Length() << '\n';
			}
		}

		PitchBy(camera, rotationVertical * dt);
	}
	{
		float up = 0.0f;
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::R)) {
			up += 1.0f;
		}
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::F)) {
			up -= 1.0f;
		}

		camera.SetHeight(camera.GetHeight() + (up * dt));
	}
}

}