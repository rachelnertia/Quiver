#pragma once

#include <array>

#include <gsl/span>

#include "Joystick.h"
#include "JoystickAxis.h"
#include "JoystickButton.h"
#include "JoystickProvider.h"

namespace qvr
{

class SfmlJoystick : public Joystick
{
public:
	struct ButtonState {
		bool isDown = false;
		bool wasDown = false;
	};

	struct AxisState {
		float position = 0.0f;
		float previousPosition = 0.0f;
	};

private:
	const int m_Index;
	
	int m_ButtonCount;
	int m_AxisCount;

	bool m_IsConnected  = false;
	bool m_WasConnected = false;

	// Copied from sf::Joystick
	enum
	{
		ButtonCount = 32, ///< Maximum number of supported buttons
		AxisCount = 8     ///< Maximum number of supported axes
	};

	std::array<ButtonState, ButtonCount> m_Buttons;
	std::array<AxisState,   AxisCount>   m_Axes;

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

	const gsl::span<const ButtonState> GetButtons() const {
		return gsl::make_span(m_Buttons.data(), m_ButtonCount);
	}

	const gsl::span<const AxisState> GetAxes() const {
		return gsl::make_span(m_Axes);
	}

	int GetButtonCount() const {
		return m_ButtonCount;
	}

	int GetAxisCount() const {
		return m_AxisCount;
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

void LogSfmlJoystickInfo(const int index);

}
