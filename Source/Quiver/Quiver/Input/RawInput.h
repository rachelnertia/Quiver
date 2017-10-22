#pragma once

#include <SFML/System/Vector2.hpp>

#include <optional.hpp>

#include "JoystickProvider.h"

namespace qvr
{

class Mouse;
class Joystick;
class Keyboard;

class RawInputDevices {
	Mouse&   m_Mouse;
	Keyboard& m_Keyboard;
	JoystickProvider& m_JoystickProvider;
public:
	RawInputDevices(
		Mouse& mouse, 
		Keyboard& keyboard,
		JoystickProvider& joystickProvider) 
	: m_Mouse(mouse)
	, m_Keyboard(keyboard)
	, m_JoystickProvider(joystickProvider)
	{}

	const Keyboard& GetKeyboard() const { return m_Keyboard; }

	Mouse&       GetMouse()       { return m_Mouse; }
	const Mouse& GetMouse() const { return m_Mouse; }

	const JoystickProvider& GetJoysticks() const { return m_JoystickProvider; }
};

}