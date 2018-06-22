#pragma once

#include "Mouse.h"

namespace sf {
class Window;
}

namespace qvr {

class SfmlMouse : public Mouse
{
public:
	SfmlMouse(sf::Window& window) : m_Window(window) {}

	void OnStep();
	void OnFrame();

	void SetMouselook(const bool enableMouselook) override;
	void SetHidden(const bool hidden) override;

private:
	sf::Window& m_Window;

	bool m_Mouselook = false;
	bool m_Hidden = false;
};

}