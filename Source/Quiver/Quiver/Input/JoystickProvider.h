#pragma once

namespace qvr
{

class Joystick;

class JoystickIndex {
	int value;
public:
	explicit JoystickIndex(const int index) : value(index) {}
	JoystickIndex(const JoystickIndex&) = default;
	JoystickIndex& operator=(const JoystickIndex&) = default;
	int Get() const { return value; }
};

class JoystickProvider
{
public:
	virtual const Joystick* GetJoystick(const JoystickIndex index) const = 0;

	// TODO? Make it possible to iterate over all the connected Joysticks
	// using nice syntax (maybe even make it STL-compliant?)
};

}