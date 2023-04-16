#include "Quiver/Application/Application.h"
#include "Quiver/Application/Config.h"

// This is the most basic possible Quiver application.

int main()
{
	qvr::ApplicationConfig config = qvr::LoadConfig("config.json");
	return qvr::RunApplication(config);
}