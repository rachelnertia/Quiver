#include "SfmlJoystick.h"

#include <SFML/Window/Joystick.hpp>
#include <spdlog/spdlog.h>

namespace qvr
{

void OnJustConnected(const int joystickIndex) {
	auto log = spdlog::get("console");
	assert(log);

	using Identification = sf::Joystick::Identification;

	const Identification identification =
		sf::Joystick::getIdentification(joystickIndex);

	log->info(
		"Joystick {} just connected, "
		"Identification: {{ Name: {}, Product Id: {}, Vendor Id: {} }}",
		joystickIndex,
		identification.name.toAnsiString(),
		identification.productId,
		identification.vendorId);

	// TODO: Log how many buttons it has
	// TODO: Log which axes it has
}

void OnJustDisconnected(const int joystickIndex) {
	auto log = spdlog::get("console");
	assert(log);

	log->info("Joystick {} just disconnected", joystickIndex);
}

void SfmlJoystick::UpdateButtons() {
	for (Button& button : m_Buttons) {
		button.wasDown = button.isDown;
	}

	for (int i = 0; i < (int)m_Buttons.size(); i++) {
		m_Buttons[i].isDown = sf::Joystick::isButtonPressed(m_Index, i);
	}
}

void SfmlJoystick::UpdateAxes() {
	for (Axis& axis : m_Axes) {
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
	}
	else if (JustDisconnected()) {
		OnJustDisconnected(m_Index);
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

}