#pragma once

#include "Quiver/Application/ApplicationState.h"

namespace qvr {

class MainMenu : public ApplicationState
{
public:
	MainMenu(ApplicationStateContext& context)
		: ApplicationState(context)
	{}

	void ProcessEvent(sf::Event& event);
	void ProcessFrame();
};

}