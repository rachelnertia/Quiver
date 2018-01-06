#include "EntityEditor.h"

#include <ImGui/imgui.h>
#include <spdlog/spdlog.h>

#include "Quiver/Entity/Entity.h"
#include "Quiver/Entity/AudioComponent/AudioComponentEditor.h"
#include "Quiver/Entity/CustomComponent/CustomComponent.h"
#include "Quiver/Entity/PhysicsComponent/PhysicsComponentEditor.h"
#include "Quiver/Entity/RenderComponent/RenderComponentEditor.h"
#include "Quiver/Misc/ImGuiHelpers.h"
#include "Quiver/World/World.h"

namespace qvr
{

EntityEditor::EntityEditor(Entity& entity) : m_Entity(entity) {}

EntityEditor::~EntityEditor() {}

void EntityEditor::GuiControls() {
	auto log = spdlog::get("console");
	assert(log);

	ImGui::Text("Entity Address: %p", (unsigned)(&m_Entity));

	ImGui::Text(
		"Prefab: %s",
		m_Entity.GetPrefab().empty() ? "None" : m_Entity.GetPrefab().c_str());

	if (ImGui::CollapsingHeader("Make Prefab")) {
		static char buffer[128];
		ImGui::InputText("Name", buffer);
		if (ImGui::Button("Save as Prefab")) {
			if (strlen(buffer) > 0) {
				World& world = m_Entity.GetWorld();

				const bool ret =
					world.mEntityPrefabs.AddPrefab(
						buffer,
						m_Entity,
						world.GetCustomComponentTypes());

				if (ret) {
					log->info("Added/updated prefab \"{}\"", buffer);
					m_Entity.mPrefabName = buffer;
				}
				else {
					log->error("Could not add/update prefab \"{}\"", buffer);
				}
			}
			else {
				log->error("No name specified.");
			}
		}
	}

	if (ImGui::CollapsingHeader("Render Component")) {
		ImGui::AutoIndent indent;

		if (!m_Entity.GetGraphics() && ImGui::Button("Add Render Component"))
		{
			m_Entity.AddGraphics();
		}

		if (m_Entity.GetGraphics())
		{
			if (!m_RenderComponentEditor ||
				!m_RenderComponentEditor->IsTargeting(*m_Entity.GetGraphics()))
			{
				m_RenderComponentEditor = std::make_unique<RenderComponentEditor>(*m_Entity.GetGraphics());
			}

			if (ImGui::CollapsingHeader("Edit Render Component"))
			{
				ImGui::AutoIndent indent2;

				m_RenderComponentEditor->GuiControls();
			}

			if (ImGui::Button("Remove Render Component"))
			{
				m_Entity.RemoveGraphics();
				m_RenderComponentEditor.release();
			}
		}
	}

	if (ImGui::CollapsingHeader("Physics Component")) {
		ImGui::AutoIndent indent;

		if (!m_PhysicsComponentEditor ||
			!m_PhysicsComponentEditor->IsTargeting(*m_Entity.GetPhysics()))
		{
			m_PhysicsComponentEditor = std::make_unique<PhysicsComponentEditor>(*m_Entity.GetPhysics());
		}

		m_PhysicsComponentEditor->GuiControls();
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
				m_AudioComponentEditor.release();
			}
		}
	}

	if (ImGui::CollapsingHeader("Custom Component")) {
		ImGui::Indent();

		CustomComponent* customComp = m_Entity.GetCustomComponent();

		if (customComp) {
			// Display custom component's type
			ImGui::Text("Type: %s", customComp->GetTypeName().c_str());
			ImGui::Separator();

			if (ImGui::Button("Remove Custom Component")) {
				m_Entity.AddCustomComponent(nullptr); // AddCustomComponent handles deletion.
			}
			else {
				if (!m_CustomComponentEditor ||
					!m_CustomComponentEditor->IsTargeting(*m_Entity.GetCustomComponent()))
				{
					m_CustomComponentEditor = customComp->CreateEditor();
				}

				if (m_CustomComponentEditor) {
					m_CustomComponentEditor->GuiControls();
				}
			}
		}
		else {
			ImGui::Text("No Custom Component");
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

			ImGui::Text("Add Custom Component:");

			int selection = -1;
			if (ImGui::ListBox("Available Types",
				&selection,
				itemsGetter,
				(void*)&typeNames,
				(int)typeNames.size()))
			{
				m_Entity.AddCustomComponent(
					m_Entity.GetWorld().GetCustomComponentTypes().GetType(typeNames[selection])
					->CreateInstance(m_Entity));
			}
		}

		ImGui::Unindent();
	}
}

}