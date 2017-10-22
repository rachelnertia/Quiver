#pragma once

namespace qvr
{

enum class JoystickAxis;

class JoystickButton;

class Joystick
{
public:
	virtual bool IsDown(const JoystickButton button) const = 0;
	virtual bool JustDown(const JoystickButton button) const = 0;
	virtual bool JustUp(const JoystickButton button) const = 0;

	virtual float GetPosition(const JoystickAxis axis) const = 0;
	virtual float GetPreviousPosition(const JoystickAxis axis) const = 0;
};

}