#pragma once

#include <memory>

#include "Quiver/Physics/PhysicsUtils.h"

namespace sf {
	class RenderWindow;
}

namespace qvr {

class CustomComponentTypeLibrary;

int RunApplication(
	CustomComponentTypeLibrary& customComponentTypes,
	FixtureFilterBitNames& fixtureFilterBitNames);
int RunApplication(CustomComponentTypeLibrary& customComponentTypes);
int RunApplication();

}