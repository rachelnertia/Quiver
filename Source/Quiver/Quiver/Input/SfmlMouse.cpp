#include "SfmlMouse.h"

#include <SFML/Window/Mouse.hpp>
#include <SFML/Window/Window.hpp>

namespace {

sf::Vector2i GetHalfSize(const sf::Window& window) {
	return sf::Vector2i(window.getSize().x / 2, window.getSize().y / 2);
}

}

namespace qvr
{

void SfmlMouse::OnStep() {
	if (m_Mouselook) {
		const sf::Vector2i windowCentre = GetHalfSize(m_Window);

		m_PositionDelta = sf::Mouse::getPosition(m_Window) - windowCentre;

		sf::Mouse::setPosition(windowCentre, m_Window);
	}
	else {
		m_PositionDelta = sf::Mouse::getPosition() - m_Position;
	}

	m_Position = sf::Mouse::getPosition();
	m_PositionRelative = sf::Mouse::getPosition(m_Window);

	for (Button& button : m_Buttons) {
		button.wasDown = button.isDown;
	}

	for (int i = 0; i < (int)m_Buttons.size(); i++) {
		m_Buttons[i].isDown = sf::Mouse::isButtonPressed((sf::Mouse::Button)i);
	}
}

void SfmlMouse::OnFrame() {
	m_Window.setMouseCursorVisible(!m_Hidden);
}

void SfmlMouse::SetMouselook(const bool enableMouselook) {
	m_Mouselook = enableMouselook;

	if (m_Mouselook) {
		const sf::Vector2i windowCentre = GetHalfSize(m_Window);;
		sf::Mouse::setPosition(windowCentre, m_Window);
	}
}

void SfmlMouse::SetHidden(const bool hidden) {
	m_Hidden = hidden;
	m_Window.setMouseCursorVisible(!hidden);
}

}