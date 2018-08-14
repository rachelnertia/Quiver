#include "PlayerInput.h"

#include "Quiver/Input/Joystick.h"
#include "Quiver/Input/JoystickAxis.h"
#include "Quiver/Input/Keyboard.h"
#include "Quiver/Input/Mouse.h"
#include "Quiver/Input/RawInput.h"

namespace Quarrel
{

b2Vec2 GetDirectionVector(const qvr::RawInputDevices& devices)
{
	auto& keyboard = devices.GetKeyboard();

	float y = 0.0f;
	y += keyboard.IsDown(qvr::KeyboardKey::W) ? 1.0f : 0.0f;
	y -= keyboard.IsDown(qvr::KeyboardKey::S) ? 1.0f : 0.0f;
	// x axis points to the left
	float x = 0.0f;
	x -= keyboard.IsDown(qvr::KeyboardKey::D) ? 1.0f : 0.0f;
	x += keyboard.IsDown(qvr::KeyboardKey::A) ? 1.0f : 0.0f;
	const qvr::JoystickIndex joystickIndex(0);
	if (devices.GetJoysticks().GetJoystick(joystickIndex)) {
		auto joystick = devices.GetJoysticks().GetJoystick(joystickIndex);

		const float joyx = (joystick->GetPosition(qvr::JoystickAxis::X) / 100.0f);
		const float joyy = (joystick->GetPosition(qvr::JoystickAxis::Y) / 100.0f);

		auto IgnoreBelowThreshold = [](const float v, const float threshold)
		{
			return abs(v) > threshold ? v : 0;
		};

		const float threshold = 0.15f;
		x -= IgnoreBelowThreshold(joyx, threshold);
		y -= IgnoreBelowThreshold(joyy, threshold);
	}
	b2Vec2 vec(x, y);
	if (vec.Length() > 1.0f) {
		vec.Normalize();
	}
	return vec;
}

float GetMouselookAngle(qvr::Mouse& mouse) {
	mouse.SetHidden(true);
	mouse.SetMouselook(true);

	const int deltaLimit = 100;

	const int mouseDeltaX =
		std::max(
			std::min(
				mouse.GetPositionDelta().x,
				deltaLimit),
			-deltaLimit);

	const float mouselookSensitivity = 0.02f;

	return mouseDeltaX * mouselookSensitivity;
}

float GetTurnAngle(qvr::RawInputDevices& devices) {
	float angle = 0.0f;

	auto& keyboard = devices.GetKeyboard();

	if (keyboard.IsDown(qvr::KeyboardKey::Right)) {
		angle += 1.0f;
	}
	if (keyboard.IsDown(qvr::KeyboardKey::Left)) {
		angle -= 1.0f;
	}

	const qvr::JoystickIndex joystickIndex(0);

	if (devices.GetJoysticks().GetJoystick(joystickIndex)) {
		auto joystick = devices.GetJoysticks().GetJoystick(joystickIndex);

		float z = joystick->GetPosition(qvr::JoystickAxis::U);

		z /= 100;

		float threshold = 0.25f;

		z = abs(z) > threshold ? z : 0;

		angle += z;
	}

	// By using the joystick and keyboard at the same time it's possible to turn
	// at double speed, unless we clamp it.
	if (angle > 1.0f) {
		angle = 1.0f;
	}
	else if (angle < -1.0f) {
		angle = -1.0f;
	}

	// Override with mouselook.
	const float mouselookAngle = GetMouselookAngle(devices.GetMouse());
	if (fabsf(mouselookAngle) > fabsf(angle)) {
		angle = mouselookAngle;
	}

	return angle;
}

}