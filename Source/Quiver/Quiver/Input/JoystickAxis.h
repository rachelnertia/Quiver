#pragma once

namespace qvr
{

// Copied from sf::Joystick::Axis
enum class JoystickAxis
{
	X,    ///< The X axis
	Y,    ///< The Y axis
	Z,    ///< The Z axis
	R,    ///< The R axis
	U,    ///< The U axis
	V,    ///< The V axis
	PovX, ///< The X axis of the point-of-view hat
	PovY, ///< The Y axis of the point-of-view hat

	Count
};

}