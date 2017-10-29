#pragma once

namespace qvr
{

class PhysicsComponent;

class PhysicsComponentEditor
{
public:
	PhysicsComponentEditor(PhysicsComponent& physicsComponent);
	~PhysicsComponentEditor();
	void GuiControls();
	bool IsTargeting(const PhysicsComponent& physicsComponent) const
	{
		return &physicsComponent == &m_PhysicsComponent;
	}
private:
	PhysicsComponent& m_PhysicsComponent;
};

}