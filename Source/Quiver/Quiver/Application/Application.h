#pragma once

#include <memory>

#include "Quiver/Physics/PhysicsUtils.h"

namespace sf {
	class RenderWindow;
}

namespace qvr {

class ApplicationStateLibrary;
class CustomComponentTypeLibrary;
struct ApplicationConfig;
struct ApplicationParams;

struct ApplicationParams {
	CustomComponentTypeLibrary& customComponentTypes;
	FixtureFilterBitNames& fixtureFilterBitNames;
	ApplicationConfig& config;
	ApplicationStateLibrary& userStates;
};

int RunApplication(ApplicationParams params);
int RunApplication(
	CustomComponentTypeLibrary& customComponentTypes,
	FixtureFilterBitNames& fixtureFilterBitNames);
int RunApplication(CustomComponentTypeLibrary& customComponentTypes);
int RunApplication();

}