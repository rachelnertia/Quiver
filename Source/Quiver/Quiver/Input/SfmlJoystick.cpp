#include "SfmlJoystick.h"

#include <SFML/Window/Joystick.hpp>

#include "Quiver/Misc/Logging.h"

namespace qvr
{

void OnJustConnected(const int joystickIndex) {
	auto log = GetConsoleLogger();

	log->info("Joystick {} just connected.", joystickIndex);

	LogSfmlJoystickInfo(joystickIndex);
}

void OnJustDisconnected(const int joystickIndex) {
	auto log = GetConsoleLogger();

	log->info("Joystick {} just disconnected", joystickIndex);
}

void SfmlJoystick::UpdateButtons() {
	for (ButtonState& button : m_Buttons) {
		button.wasDown = button.isDown;
	}

	for (int i = 0; i < (int)m_Buttons.size(); i++) {
		m_Buttons[i].isDown = sf::Joystick::isButtonPressed(m_Index, i);
	}
}

void SfmlJoystick::UpdateAxes() {
	for (AxisState& axis : m_Axes) {
		axis.previousPosition = axis.position;
	}

	for (int i = 0; i < (int)m_Axes.size(); i++) {
		m_Axes[i].position = sf::Joystick::getAxisPosition(m_Index, (sf::Joystick::Axis)i);
	}
}

void SfmlJoystick::Update() {
	m_WasConnected = m_IsConnected;
	m_IsConnected = sf::Joystick::isConnected(m_Index);
	if (JustConnected()) {
		OnJustConnected(m_Index);
		m_ButtonCount = sf::Joystick::getButtonCount(m_Index);
		m_AxisCount = 0;
		for (int i = 0; i < sf::Joystick::AxisCount; i++) {
			if (sf::Joystick::hasAxis(m_Index, (sf::Joystick::Axis)i)) {
				m_AxisCount++;
			}
		}
	}
	else if (JustDisconnected()) {
		OnJustDisconnected(m_Index);
		m_ButtonCount = 0;
		m_AxisCount = 0;
	}

	UpdateButtons();
	UpdateAxes();
}

void SfmlJoystickSet::Update() {
	sf::Joystick::update();

	for (SfmlJoystick& joystick : m_Joysticks) {
		joystick.Update();
	}
}

void LogSfmlJoystickInfo(const int index) {
	auto log = GetConsoleLogger();

	using Identification = sf::Joystick::Identification;

	const Identification identification =
		sf::Joystick::getIdentification(index);

	log->info(
		"Identification: {{ Name: {}, Product Id: {}, Vendor Id: {} }}",
		identification.name.toAnsiString(),
		identification.productId,
		identification.vendorId);

	// TODO: Log how many buttons it has
	// TODO: Log which axes it has
}

}