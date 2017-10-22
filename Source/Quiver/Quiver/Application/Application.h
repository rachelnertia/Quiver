#pragma once

#include <memory>

namespace sf {
	class RenderWindow;
}

namespace qvr {

class CustomComponentTypeLibrary;

int RunApplication(CustomComponentTypeLibrary& customComponentTypes);
int RunApplication();

}