#pragma once

#include "Quiver/Application/ApplicationState.h"

namespace qvr {

class MainMenu : public ApplicationState
{
public:
	MainMenu(ApplicationStateContext& context)
		: ApplicationState(context)
	{}

	void ProcessFrame();
};

}