#pragma once

#include "JoystickAxis.h"

namespace qvr {

namespace Xbox360Controller
{

enum class Button
{
	A = 0,
	B = 1,
	X = 2,
	Y = 3,
	Bumper_Left = 4,
	Bumper_Right = 5,
	Back = 6,
	Start = 7,
	AnalogStick_Left_Click = 8,
	AnalogStick_Right_Click = 9
};

enum class Axis
{
	AnalogStick_Left_Horizontal,
	AnalogStick_Left_Vertical,
	AnalogStick_Right_Horizontal,
	AnalogStick_Right_Vertical,
	Trigger_Left,
	Trigger_Right,
	DPad_Horizontal,
	DPad_Vertical
};

inline qvr::JoystickAxis ToJoystickAxis(const Axis axis)
{
	switch (axis)
	{
	case Axis::AnalogStick_Left_Horizontal:  return qvr::JoystickAxis::X;
	case Axis::AnalogStick_Left_Vertical:    return qvr::JoystickAxis::Y;
	case Axis::AnalogStick_Right_Horizontal: return qvr::JoystickAxis::U;
	case Axis::AnalogStick_Right_Vertical:   return qvr::JoystickAxis::R;
		// Disgustingly, the triggers share an axis, with right being negative
		// and left being positive. This makes it impossible to use both at the same time.
		// TODO: Figure out a good!
	case Axis::Trigger_Left:                 return qvr::JoystickAxis::Z;
	case Axis::Trigger_Right:                return qvr::JoystickAxis::Z;
	case Axis::DPad_Horizontal:              return qvr::JoystickAxis::PovX;
	case Axis::DPad_Vertical:                return qvr::JoystickAxis::PovY;
	}
	return qvr::JoystickAxis::X;
}

}

}