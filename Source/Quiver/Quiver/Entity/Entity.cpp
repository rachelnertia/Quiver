#include "Entity.h"

#include <Box2D/Dynamics/b2Body.h>
#include <Box2D/Dynamics/b2Fixture.h>
#include <Box2D/Dynamics/b2World.h>
#include <Box2D/Collision/Shapes/b2PolygonShape.h>
#include <ImGui/imgui.h>

#include "Quiver/Entity/AudioComponent/AudioComponent.h"
#include "Quiver/Entity/CustomComponent/CustomComponent.h"
#include "Quiver/Entity/PhysicsComponent/PhysicsComponent.h"
#include "Quiver/Entity/PhysicsComponent/PhysicsComponentDef.h"
#include "Quiver/Entity/RenderComponent/RenderComponent.h"
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
	using json = nlohmann::json;

	if (!GetPhysics()) {
		std::cout << "Entity has no PhysicsComponent!" << std::endl;
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
			std::cout << "Couldn't serialize RenderComponent." << std::endl;
			return false;
		}

		j["RenderComponent"] = renderData;
	}

	{
		const json physicsData = GetPhysics()->ToJson();
		if (physicsData.empty()) {
			std::cout << "Couldn't serialize PhysicsComponent." << std::endl;
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
	constexpr const char* logContext = "Entity::FromJson: ";

	using json = nlohmann::json;

	if (!VerifyJson(j, world.GetCustomComponentTypes())) {
		std::cout << "Entity JSON is invalid." << std::endl;
		return nullptr;
	}

	// Determine if this is a instance of a prefab.
	if (j.find("PrefabName") != j.end()) {
		if (!j["PrefabName"].is_string()) {
			std::cout << logContext << "\"PrefabName\" field found, but it is not a string.\n";
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

EntityEditor::EntityEditor(Entity& entity) : m_Entity(entity) {}
EntityEditor::~EntityEditor() {}

void EntityEditor::GuiControls() {
	ImGui::Text("Entity Address: %p", (unsigned)(&m_Entity));

	ImGui::Text(
		"Prefab: %s",
		m_Entity.GetPrefab().empty() ? "None" : m_Entity.GetPrefab().c_str());

	if (ImGui::CollapsingHeader("Make Prefab")) {
		const size_t bufferSize = 128;
		static char buffer[bufferSize];
		ImGui::InputText("Name", buffer, bufferSize);
		if (ImGui::Button("Save as Prefab")) {
			if (strlen(buffer) > 0) {
				World& world = m_Entity.GetWorld();

				const bool ret =
					world.mEntityPrefabs.AddPrefab(
						buffer,
						m_Entity,
						world.GetCustomComponentTypes());

				if (ret) {
					std::cout << "Added/updated prefab \"" << buffer << "\".\n";
					m_Entity.mPrefabName = buffer;
				}
				else {
					std::cout << "Could not add/update prefab \"" << buffer << "\".\n";
				}
			}
			else {
				std::cout << "No name specified.\n";
			}
		}
	}

	if (ImGui::CollapsingHeader("Render Component")) {
		ImGui::AutoIndent indent;

		if (m_Entity.GetGraphics()) {
			m_Entity.GetGraphics()->GuiControls();

			ImGui::NewLine();

			if (ImGui::Button("Remove Render Component")) {
				m_Entity.RemoveGraphics();
			}
		}
		else {
			if (ImGui::Button("Add Render Component")) {
				m_Entity.AddGraphics();
			}
		}
	}

	if (ImGui::CollapsingHeader("Physics Component")) {
		ImGui::AutoIndent indent;

		m_Entity.GetPhysics()->GuiControls();
	}

	if (ImGui::CollapsingHeader("Audio Component"))
	{
		ImGui::AutoIndent indent;

		if (!m_Entity.GetAudio() && ImGui::Button("Add Audio Component"))
		{
			m_Entity.AddAudio();
		}

		if (m_Entity.GetAudio())
		{
			if (!m_AudioComponentEditor ||
				!m_AudioComponentEditor->IsTargeting(*m_Entity.GetAudio()))
			{
				m_AudioComponentEditor = std::make_unique<AudioComponentEditor>(*m_Entity.GetAudio());
			}

			if (ImGui::CollapsingHeader("Edit Audio Component"))
			{
				ImGui::AutoIndent indent2;

				m_AudioComponentEditor->GuiControls();
			}

			if (ImGui::Button("Remove Audio Component"))
			{
				m_Entity.RemoveAudio();
				// Don't need to do this, but may as well:
				m_AudioComponentEditor.release();
			}
		}
	}

	if (ImGui::CollapsingHeader("Input Component")) {
		ImGui::Indent();

		CustomComponent* inputComp = m_Entity.GetCustomComponent();

		if (inputComp) {
			// Display input component's type
			ImGui::Text("Type: %s", inputComp->GetTypeName().c_str());
			ImGui::Separator();

			if (ImGui::Button("Remove Input Component")) {
				m_Entity.SetInput(nullptr); // SetInput handles deletion.
			}
			else {
				// Display controls
				inputComp->GUIControls();
			}

		}
		else {
			ImGui::Text("No Input Component");
			ImGui::Separator();

			// List of the names of available CustomComponent types
			// (Child classes of CustomComponent)
			std::vector<std::string> typeNames =
				m_Entity.GetWorld().GetCustomComponentTypes().GetTypeNames();

			auto itemsGetter =
				[](void* data, int index, const char** outText) -> bool
			{
				std::vector<std::string>* stringArray = (std::vector<std::string>*)data;

				if (index >= (int)stringArray->size()) return false;
				if (index < 0) return false;

				*outText = stringArray->at(index).c_str();

				return true;
			};

			ImGui::Text("Add Input Component:");

			int selection = -1;
			if (ImGui::ListBox("Available Types",
				&selection,
				itemsGetter,
				(void*)&typeNames,
				(int)typeNames.size()))
			{
				m_Entity.SetInput(
					m_Entity.GetWorld().GetCustomComponentTypes().GetType(typeNames[selection])
					->CreateInstance(m_Entity));
			}
		}

		ImGui::Unindent();
	}
}

}