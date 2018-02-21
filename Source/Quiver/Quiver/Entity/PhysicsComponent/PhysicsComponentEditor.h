#pragma once

#include "Quiver/Physics/PhysicsUtils.h"

namespace qvr
{

class PhysicsComponent;

class PhysicsComponentEditor
{
public:
	PhysicsComponentEditor(PhysicsComponent& physicsComponent);
	
	~PhysicsComponentEditor();
	
	void GuiControls(const FixtureFilterBitNames& bitNames);
	
	bool IsTargeting(const PhysicsComponent& physicsComponent) const
	{
		return &physicsComponent == &m_PhysicsComponent;
	}
private:
	PhysicsComponent& m_PhysicsComponent;
};

}