#pragma once

#include <memory>

namespace qvr
{

class Entity;
class AudioComponentEditor;
class CustomComponentEditor;
class PhysicsComponentEditor;
class RenderComponentEditor;

class EntityEditor
{
public:
	EntityEditor(Entity& entity);
	
	~EntityEditor();
	
	void GuiControls();
	
	bool IsTargeting(const Entity& entity) const
	{
		return &entity == &m_Entity;
	}

private:
	Entity& m_Entity;
	
	// Maybe this could be an optional.
	std::unique_ptr<AudioComponentEditor>   m_AudioComponentEditor;
	std::unique_ptr<PhysicsComponentEditor> m_PhysicsComponentEditor;
	std::unique_ptr<RenderComponentEditor>  m_RenderComponentEditor;
	std::unique_ptr<CustomComponentEditor>  m_CustomComponentEditor;
};

}