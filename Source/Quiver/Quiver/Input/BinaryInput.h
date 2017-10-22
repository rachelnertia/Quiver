#pragma once

#include <vector>

#include "Mouse.h"
#include "Joystick.h"
#include "JoystickButton.h"
#include "Keyboard.h"
#include "RawInput.h"
#include "Xbox360Controller.h"

namespace qvr
{

class JoystickAxisThreshold
{
public:
	JoystickAxisThreshold(
		const qvr::JoystickAxis axis,
		const float threshold,
		const bool trueAboveThreshold)
		: mJoystickAxis(axis)
		, mThreshold(threshold)
		, mTrueAboveThreshold(trueAboveThreshold)
	{}
	JoystickAxisThreshold(
		const Xbox360Controller::Axis axis,
		const float threshold,
		const bool trueAboveThreshold)
		: JoystickAxisThreshold(
			Xbox360Controller::ToJoystickAxis(axis),
			threshold,
			trueAboveThreshold)
	{
	}
	bool IsActive(const qvr::Joystick& joystick) const
	{
		const float v = joystick.GetPosition(mJoystickAxis);
		if (mTrueAboveThreshold)
		{
			return v > mThreshold;
		}
		return v < mThreshold;
	}
	bool JustActive(const qvr::Joystick& joystick) const
	{
		const float current  = joystick.GetPosition(mJoystickAxis);
		const float previous = joystick.GetPreviousPosition(mJoystickAxis);
		if (mTrueAboveThreshold)
		{
			return current > mThreshold && previous < mThreshold;
		}
		return current < mThreshold && previous > mThreshold;
	}
private:
	const qvr::JoystickAxis mJoystickAxis;
	const float mThreshold;
	const bool mTrueAboveThreshold;
};

}

class BinaryInput
{
public:
	enum class Type
	{
		KeyboardKey,
		MouseButton,
		JoystickButton,
		JoystickAxisThreshold
	};

	BinaryInput(qvr::KeyboardKey key)
		: mType(Type::KeyboardKey)
		, mKeyboardKey(key)
	{}

	BinaryInput(qvr::MouseButton mouseButton)
		: mType(Type::MouseButton)
		, mMouseButton(mouseButton)
	{}

	BinaryInput(qvr::JoystickButton joystickButton)
		: mType(Type::JoystickButton)
		, mJoystickButton(joystickButton)
	{}

	BinaryInput(qvr::JoystickAxisThreshold joystickAxisThreshold)
		: mType(Type::JoystickAxisThreshold)
		, mJoystickAxisThreshold(joystickAxisThreshold)
	{}

	bool IsActive(const qvr::RawInputDevices& devices) const
	{
		const auto joystickIndex = qvr::JoystickIndex(0);
		const qvr::Joystick* joystick = devices.GetJoysticks().GetJoystick(joystickIndex);

		switch (mType)
		{
		case Type::KeyboardKey:
			return devices.GetKeyboard().IsDown(mKeyboardKey);
		case Type::MouseButton:
			return devices.GetMouse().IsDown(mMouseButton);
		// TODO: Which joysticks?
		case Type::JoystickButton:
			if (joystick) {
				return joystick->IsDown(mJoystickButton);
			}
			break;
		case Type::JoystickAxisThreshold:
			if (joystick) {
				return mJoystickAxisThreshold.IsActive(*joystick);
			}
			break;
		}
		return false;
	}

	bool JustActive(const qvr::RawInputDevices& devices) const
	{
		const auto joystickIndex = qvr::JoystickIndex(0);
		const qvr::Joystick* joystick = devices.GetJoysticks().GetJoystick(joystickIndex);

		switch (mType)
		{
		case Type::KeyboardKey:
			return devices.GetKeyboard().JustDown(mKeyboardKey);
		case Type::MouseButton:
			return devices.GetMouse().JustDown(mMouseButton);
			// TODO: Which joysticks?
		case Type::JoystickButton:
			if (joystick) {
				return joystick->JustDown(mJoystickButton);
			}
			break;
		case Type::JoystickAxisThreshold:
			if (joystick) {
				return mJoystickAxisThreshold.JustActive(*joystick);
			}
			break;
		}
		return false;
	}

private:
	const Type mType;

	union
	{
		const qvr::KeyboardKey mKeyboardKey;
		const qvr::MouseButton mMouseButton;
		const qvr::JoystickButton mJoystickButton;
		const qvr::JoystickAxisThreshold mJoystickAxisThreshold;
	};
};

bool AnyActive(const qvr::RawInputDevices& devices, const std::vector<BinaryInput> inputs) {
	for (const BinaryInput input : inputs) {
		if (input.IsActive(devices)) {
			return true;
		}
	}

	return false;
}

bool AnyJustActive(const qvr::RawInputDevices& devices, const std::vector<BinaryInput> inputs) {
	for (const BinaryInput input : inputs) {
		if (input.JustActive(devices)) {
			return true;
		}
	}

	return false;
}