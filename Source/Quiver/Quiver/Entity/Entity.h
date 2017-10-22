#pragma once

#include "json.hpp"

struct b2Vec2;
struct b2Transform;
class b2Shape;

namespace qvr {

class AudioComponent;
class EntityEditor;
class CustomComponent;
class CustomComponentTypeLibrary;
class PhysicsComponent;
class RenderComponent;
class World;
struct PhysicsComponentDef;

class Entity {
public:
	Entity(World& world, const PhysicsComponentDef& physicsDef);
	~Entity();

	Entity(const Entity&) = delete;
	Entity(const Entity&&) = delete;

	Entity& operator=(const Entity&) = delete;
	Entity& operator=(const Entity&&) = delete;

	nlohmann::json ToJson(const bool toPrefab = false) const;
	
	static std::unique_ptr<Entity> FromJson(World& world, const nlohmann::json & j);

	static bool VerifyJson(
		const nlohmann::json & j, 
		const CustomComponentTypeLibrary& customComponentTypes);

	void SetInput(std::unique_ptr<CustomComponent> newInput);

	void AddGraphics();                                          // Add a RenderComponent.
	void AddGraphics(const nlohmann::json& renderComponentJson); // Add a RenderComponent from JSON.
	void RemoveGraphics();                                       // Remove the RenderComponent.

	void AddAudio();
	void RemoveAudio();

	AudioComponent*   GetAudio()           const { return mAudioComponent.get(); }
	CustomComponent*  GetCustomComponent() const { return mCustomComponent.get(); }
	PhysicsComponent* GetPhysics()         const { return mPhysicsComponent.get(); }
	RenderComponent*  GetGraphics()        const { return mRenderComponent.get(); }
	
	World& GetWorld() const { return mWorld; }
	
	std::string GetPrefab() const { return mPrefabName; }

	void SetPrefab(std::string prefabName) { mPrefabName = prefabName; };

private:
	friend class EntityEditor;

	World& mWorld;
	
	std::unique_ptr<PhysicsComponent> mPhysicsComponent;
	std::unique_ptr<RenderComponent> mRenderComponent;	
	std::unique_ptr<AudioComponent> mAudioComponent;
	std::unique_ptr<CustomComponent> mCustomComponent;

	std::string mPrefabName;
};

class AudioComponentEditor;

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
	std::unique_ptr<AudioComponentEditor> m_AudioComponentEditor;
};

}