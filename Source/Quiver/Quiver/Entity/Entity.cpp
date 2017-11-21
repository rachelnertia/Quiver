#include "Entity.h"

#include <Box2D/Dynamics/b2Body.h>
#include <Box2D/Dynamics/b2Fixture.h>
#include <Box2D/Dynamics/b2World.h>
#include <Box2D/Collision/Shapes/b2PolygonShape.h>
#include <ImGui/imgui.h>
#include <spdlog/spdlog.h>

#include "Quiver/Entity/AudioComponent/AudioComponent.h"
#include "Quiver/Entity/CustomComponent/CustomComponent.h"
#include "Quiver/Entity/PhysicsComponent/PhysicsComponent.h"
#include "Quiver/Entity/PhysicsComponent/PhysicsComponentDef.h"
#include "Quiver/Entity/PhysicsComponent/PhysicsComponentEditor.h"
#include "Quiver/Entity/RenderComponent/RenderComponent.h"
#include "Quiver/Entity/RenderComponent/RenderComponentEditor.h"
#include "Quiver/Misc/ImGuiHelpers.h"
#include "Quiver/World/World.h"

namespace qvr {

Entity::Entity(World& world, const PhysicsComponentDef& physicsDef)
	: mWorld(world)
	, mPhysicsComponent(std::make_unique<PhysicsComponent>(*this, physicsDef))
{}

Entity::~Entity() {}

nlohmann::json Entity::ToJson(const bool toPrefab) const
{
	auto log = spdlog::get("console");
	assert(log);

	const char* logCtx = "Entity::ToJson: ";

	using json = nlohmann::json;

	if (!GetPhysics()) {
		log->error("{} Entity has no PhysicsComponent!", logCtx);
		return {};
	}

	json j;

	if (!toPrefab && !mPrefabName.empty())
	{
		// This is a prefab instance.
		if (const auto prefab = GetWorld().mEntityPrefabs.GetPrefab(mPrefabName))
		{
			json thisJson = ToJson(true);

			j["Diff"] = json::diff(*prefab, thisJson);

			j["PrefabName"] = mPrefabName;

			return j;
		}

		return {};
	}

	if (GetGraphics()) {
		json renderData;

		bool result = GetGraphics()->ToJson(renderData);
		if (!result) {
			log->error("{} Couldn't serialize RenderComponent.", logCtx);
			return {};
		}

		j["RenderComponent"] = renderData;
	}

	{
		const json physicsData = GetPhysics()->ToJson();
		if (physicsData.empty()) {
			log->error("{} Couldn't serialize PhysicsComponent.", logCtx);
			return {};
		}

		j["PhysicsComponent"] = physicsData;
	}

	if (GetCustomComponent()) {
		json customComponentData;
		customComponentData["Type"] = GetCustomComponent()->GetTypeName();
		customComponentData["Data"] = GetCustomComponent()->ToJson();
		j["CustomComponent"] = customComponentData;
	}

	return j;
}

std::unique_ptr<Entity> Entity::FromJson(World& world, const nlohmann::json & j)
{
	auto log = spdlog::get("console");
	assert(log);
	const char* logContext = "Entity::FromJson:";

	using json = nlohmann::json;

	if (!VerifyJson(j, world.GetCustomComponentTypes())) {
		log->error("{} Entity JSON is invalid.", logContext);
		return nullptr;
	}

	// Determine if this is a instance of a prefab.
	if (j.find("PrefabName") != j.end()) {
		if (!j["PrefabName"].is_string()) {
			log->error("{} \"PrefabName\" field found, but it is not a string.", logContext);
			return nullptr;
		}

		const std::string prefabName = j["PrefabName"];

		const auto prefab = world.mEntityPrefabs.GetPrefab(prefabName);

		if (!prefab)
		{
			return nullptr;
		}

		std::unique_ptr<Entity> entity = FromJson(world, (*prefab).patch(j["Diff"]));

		if (!entity)
		{
			return nullptr;
		}

		entity->mPrefabName = prefabName;

		return entity;
	}

	PhysicsComponentDef physicsCompDef(j["PhysicsComponent"]);

	std::unique_ptr<Entity> entity = std::make_unique<Entity>(world, physicsCompDef);

	if (j.count("RenderComponent") > 0)
	{
		entity->AddGraphics(j["RenderComponent"]);
	}

	if (j.count("CustomComponent") > 0)
	{
		entity->SetInput(
			world.GetCustomComponentTypes().CreateInstance(*entity.get(), j["CustomComponent"]));
	}

	return entity;
}

bool Entity::VerifyJson(
	const nlohmann::json & j,
	const CustomComponentTypeLibrary& customComponentTypes)
{
	if (j.empty()) {
		return false;
	}

	if (!j.is_object()) {
		return false;
	}

	if (j.find("PrefabName") != j.end()) {
		// TODO: Try to find a Prefab with this name.
		return true;
	}

	if (j.find("PhysicsComponent") == j.end()) {
		return false;
	}

	if (!PhysicsComponentDef::VerifyJson(j["PhysicsComponent"])) {
		return false;
	}

	if (j.count("RenderComponent") > 0) {
		if (!RenderComponent::VerifyJson(j["RenderComponent"])) {
			return false;
		}
	}

	if (j.find("CustomComponent") != j.end()) {
		if (!customComponentTypes.IsValid(j["CustomComponent"])) {
			return false;
		}
	}

	return true;
}

void Entity::SetInput(std::unique_ptr<CustomComponent> newCustomComponent)
{
	mCustomComponent.reset(newCustomComponent.release());
}

void Entity::AddGraphics()
{
	assert(mRenderComponent == nullptr);

	mRenderComponent = std::make_unique<RenderComponent>(*this);
}

void Entity::AddGraphics(const nlohmann::json & renderComponentJson)
{
	AddGraphics();

	if (!mRenderComponent->FromJson(renderComponentJson))
	{
		RemoveGraphics();
	}
}

void Entity::RemoveGraphics()
{
	assert(mRenderComponent != nullptr);

	mRenderComponent.reset();
}

void Entity::AddAudio()
{
	assert(mAudioComponent == nullptr);

	mAudioComponent = std::make_unique<AudioComponent>(*this);
}

void Entity::RemoveAudio()
{
	assert(mAudioComponent != nullptr);

	mAudioComponent.reset();
}

}