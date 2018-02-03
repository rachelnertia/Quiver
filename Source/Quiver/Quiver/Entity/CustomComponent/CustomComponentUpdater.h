#pragma once

#include <chrono>
#include <vector>

namespace qvr {

class CustomComponent;
class RawInputDevices;

class CustomComponentUpdater
{
public:
	void Update(const std::chrono::duration<float> deltaTime, qvr::RawInputDevices& inputDevices);
	bool Register(CustomComponent& customComponent);
	bool Unregister(CustomComponent& customComponent);
	bool IsCurrentlyUpdating() const { return m_Index >= 0; }
	auto GetRemoveFlaggers() const -> std::vector<std::reference_wrapper<CustomComponent>>;
private:
	int m_Index = -1;
	std::vector<std::reference_wrapper<CustomComponent>> m_CustomComponents;
};

}