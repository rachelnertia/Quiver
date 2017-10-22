#pragma once

#include "Xbox360Controller.h"

namespace qvr
{

class JoystickButton
{
public:
	JoystickButton(unsigned value) : mValue(value) {}
	JoystickButton(Xbox360Controller::Button xbButton) : mValue((unsigned)xbButton) {}
	unsigned GetValue() const { return mValue; }
private:
	const unsigned mValue;
};

}