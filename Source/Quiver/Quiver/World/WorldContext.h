#pragma once

#include <memory>

#include "Quiver/Physics/PhysicsUtils.h"

namespace qvr
{

class CustomComponentTypeLibrary;

// Contains data shared by multiple Worlds.
class WorldContext
{
public:
	WorldContext(
		CustomComponentTypeLibrary& customComponentTypes,
		const FixtureFilterBitNames& filterBitNames)
		: m_CustomComponentTypes(customComponentTypes)
		, m_FilterBitNames(filterBitNames)
	{}

	CustomComponentTypeLibrary& GetCustomComponentTypes() {
		return m_CustomComponentTypes;
	}

	const FixtureFilterBitNames& GetFixtureFilterBitNames() {
		return m_FilterBitNames;
	}

private:
	CustomComponentTypeLibrary& m_CustomComponentTypes;
	const FixtureFilterBitNames& m_FilterBitNames;

};

}