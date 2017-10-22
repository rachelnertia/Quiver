#include "CustomComponentUpdater.h"
#include "CustomComponent.h"

#include "Quiver/Misc/FindByAddress.h"

namespace qvr {

void CustomComponentUpdater::Update(const float deltaSeconds, qvr::RawInputDevices& inputDevices)
{
	for (m_Index = 0; m_Index < (int)m_CustomComponents.size(); m_Index++)
	{
		m_CustomComponents[m_Index].get().HandleInput(inputDevices, deltaSeconds);
		m_CustomComponents[m_Index].get().OnStep(deltaSeconds);
	}

	m_Index = -1;
}

bool CustomComponentUpdater::Register(CustomComponent& customComponent)
{
	const auto it = FindByAddress(m_CustomComponents, customComponent);

	if (it != m_CustomComponents.end())
	{
		return true;
	}

	m_CustomComponents.push_back(customComponent);

	return true;
}

bool CustomComponentUpdater::Unregister(CustomComponent& customComponent)
{
	const auto it = FindByAddress(m_CustomComponents, customComponent);

	if (it != m_CustomComponents.end())
	{
		m_CustomComponents.erase(it);

		if (IsCurrentlyUpdating()) {
			m_Index = std::min(0, m_Index - 1);
		}

		return true;
	}

	return false;
}

}