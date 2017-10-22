#pragma once

#include <array>

#include "Joystick.h"
#include "JoystickAxis.h"
#include "JoystickButton.h"
#include "JoystickProvider.h"

namespace qvr
{

class SfmlJoystick : public Joystick
{
	const int m_Index;
	
	bool m_IsConnected  = false;
	bool m_WasConnected = false;

	struct Button {
		bool isDown  = false;
		bool wasDown = false;
	};

	struct Axis {
		float position = 0.0f;
		float previousPosition = 0.0f;
	};

	// Copied from sf::Joystick
	enum
	{
		ButtonCount = 32, ///< Maximum number of supported buttons
		AxisCount = 8   ///< Maximum number of supported axes
	};

	std::array<Button, ButtonCount> m_Buttons;
	std::array<Axis,   AxisCount>   m_Axes;

	bool JustConnected() const {
		return m_IsConnected && !m_WasConnected;
	}
	bool JustDisconnected() const {
		return m_WasConnected && !m_IsConnected;
	}

	void UpdateButtons();
	void UpdateAxes();

public:
	SfmlJoystick(const int index) : m_Index(index) {}

	void Update();

	bool IsConnected() const {
		return m_IsConnected;
	}

	bool IsDown(const JoystickButton button) const override {
		return m_Buttons[button.GetValue()].isDown;
	}
	bool JustDown(const JoystickButton button) const override {
		return m_Buttons[button.GetValue()].isDown && !m_Buttons[button.GetValue()].wasDown;
	}
	bool JustUp(const JoystickButton button) const override {
		return m_Buttons[button.GetValue()].wasDown && !m_Buttons[button.GetValue()].isDown;
	}
	
	float GetPosition(const JoystickAxis axis) const override {
		return m_Axes[(int)axis].position;
	}
	float GetPreviousPosition(const JoystickAxis axis) const override {
		return m_Axes[(int)axis].previousPosition;
	}
};

class SfmlJoystickSet : public JoystickProvider
{
public:
	// Worst constructor ever.
	SfmlJoystickSet()
		: m_Joysticks({ 0, 1, 2, 3, 4, 5, 6, 7 })
	{}

	void Update();

	const Joystick* GetJoystick(const JoystickIndex index) const override {
		if (m_Joysticks[index.Get()].IsConnected()) {
			return &m_Joysticks[index.Get()];
		}
		return nullptr;
	}

	static const int Count = 8; // The maximum number of supported joysticks.

private:
	std::array<SfmlJoystick, Count> m_Joysticks;
};

}
