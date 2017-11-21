#pragma once

#include <memory>

namespace qvr
{

class CustomComponentTypeLibrary;

// Contains data shared by multiple Worlds.
class WorldContext
{
public:
	WorldContext(CustomComponentTypeLibrary& customComponentTypes)
		: m_CustomComponentTypes(customComponentTypes)
	{}

	CustomComponentTypeLibrary& GetCustomComponentTypes() {
		return m_CustomComponentTypes;
	}

private:
	CustomComponentTypeLibrary& m_CustomComponentTypes;

};

}