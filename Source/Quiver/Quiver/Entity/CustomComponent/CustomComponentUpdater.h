#pragma once

#include <vector>

namespace qvr {

class CustomComponent;
class RawInputDevices;

class CustomComponentUpdater
{
public:
	void Update(const float deltaSeconds, qvr::RawInputDevices& inputDevices);
	bool Register(CustomComponent& customComponent);
	bool Unregister(CustomComponent& customComponent);
	bool IsCurrentlyUpdating() const { return m_Index >= 0; }
private:
	int m_Index = -1;
	std::vector<std::reference_wrapper<CustomComponent>> m_CustomComponents;
};

}