#pragma once

#include <array>

#include <SFML/System/Vector2.hpp>

namespace qvr {

// Copied from sf::Mouse::Button
enum class MouseButton
{
	Left,       ///< The left mouse button
	Right,      ///< The right mouse button
	Middle,     ///< The middle (wheel) mouse button
	XButton1,   ///< The first extra mouse button
	XButton2,   ///< The second extra mouse button

	ButtonCount ///< Keep last -- the total number of mouse buttons
};

class Mouse
{
public:
	sf::Vector2i GetPosition() const;
	sf::Vector2i GetPositionRelative() const;
	sf::Vector2i GetPositionDelta() const;

	bool IsDown(const MouseButton button) const;
	bool JustDown(const MouseButton button) const;
	bool JustUp(const MouseButton button) const;

	virtual void SetMouselook(const bool enableMouselook) {};
	virtual void SetHidden(const bool hidden) {};

protected:
	sf::Vector2i m_Position;
	sf::Vector2i m_PositionRelative;
	sf::Vector2i m_PositionDelta;

	struct Button {
		bool isDown = false, wasDown = false;
	};

	std::array<Button, (size_t)MouseButton::ButtonCount> m_Buttons;
};

inline sf::Vector2i Mouse::GetPosition() const {
	return m_Position;
}

inline sf::Vector2i Mouse::GetPositionRelative() const {
	return m_PositionRelative;
}

inline sf::Vector2i Mouse::GetPositionDelta() const {
	return m_PositionDelta;
}

inline bool Mouse::IsDown(const MouseButton button) const {
	return m_Buttons[int(button)].isDown;
}

inline bool Mouse::JustDown(const MouseButton button) const {
	return m_Buttons[int(button)].isDown && !m_Buttons[int(button)].wasDown;
}

inline bool Mouse::JustUp(const MouseButton button) const {
	return m_Buttons[int(button)].wasDown && !m_Buttons[int(button)].isDown;
}

}