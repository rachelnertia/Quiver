#include "PhysicsComponentEditor.h"

#include <Box2D/Dynamics/b2Body.h>
#include <Box2D/Dynamics/b2Fixture.h>
#include <ImGui/imgui.h>
#include <spdlog/fmt/fmt.h>

#include "Quiver/Entity/PhysicsComponent/PhysicsComponent.h"
#include "Quiver/Misc/ImGuiHelpers.h"

namespace qvr
{

bool GuiControls(b2Filter& filter)
{
	bool modified = false;

	int temp = filter.groupIndex;

	modified |= ImGui::SliderInt(
		"Group Index",
		&temp,
		0, 
		100);

	if (modified) {
		filter.groupIndex = (int16)temp;
	}

	if (ImGui::CollapsingHeader("Category Bits")) {
		ImGui::AutoIndent indent;
	
		for (int i = 0; i < 10; i++) {
			modified |= ImGui::CheckboxFlags(
				fmt::format("Bit {} ({})", i, 1 << i).c_str(), 
				(unsigned*)&filter.categoryBits, 
				1 << i);
		}
	}

	if (ImGui::CollapsingHeader("Mask Bits")) {
		ImGui::AutoIndent indent;

		for (int i = 0; i < 10; i++) {
			modified |= ImGui::CheckboxFlags(
				fmt::format("Bit {} ({})", i, 1 << i).c_str(),
				(unsigned*)&filter.maskBits,
				1 << i);
		}
	}

	return modified;
}

void GuiControls(b2Fixture& fixture)
{
	ImGui::SliderFloat(
		"Friction", 
		fixture, 
		&b2Fixture::GetFriction, 
		&b2Fixture::SetFriction, 
		0.0f, 
		1.0f);

	ImGui::SliderFloat(
		"Restitution",
		fixture,
		&b2Fixture::GetRestitution,
		&b2Fixture::SetRestitution,
		0.0f,
		2.0f);

	if (ImGui::CollapsingHeader("Filter Data"))
	{
		ImGui::AutoIndent indent;

		b2Filter filter = fixture.GetFilterData();

		if (GuiControls(filter)) {
			fixture.SetFilterData(filter);
		}
	}
}

void GuiControls(b2Body& body)
{
	{
		b2BodyType type = body.GetType();
		const char* bodyTypeNames[3] =
		{
			"Static",    // b2_staticBody
			"Kinematic", // b2_kinematicBody
			"Dynamic"    // b2_dynamicBody
		};

		if (ImGui::ListBox("Body Type",
			(int*)&type,
			bodyTypeNames,
			3))
		{
			body.SetType(type);
		}
	}

	ImGui::Checkbox(
		"Fixed Rotation", 
		body, 
		&b2Body::IsFixedRotation, 
		&b2Body::SetFixedRotation);
	
	ImGui::SliderFloat(
		"Linear Damping",
		body,
		&b2Body::GetLinearDamping,
		&b2Body::SetLinearDamping,
		0.0f,
		10.0f);

	ImGui::InputFloat(
		"Angular Damping", 
		body, 
		&b2Body::GetAngularDamping, 
		&b2Body::SetAngularDamping);

	b2Fixture* fixture = body.GetFixtureList();

	while (fixture) {
		if (ImGui::CollapsingHeader("Fixture")) {
			ImGui::AutoIndent indent;
			GuiControls(*fixture);
		}
		fixture = fixture->GetNext();
	}
}

PhysicsComponentEditor::PhysicsComponentEditor(PhysicsComponent& physicsComponent)
	: m_PhysicsComponent(physicsComponent)
{}

PhysicsComponentEditor::~PhysicsComponentEditor() {}

void PhysicsComponentEditor::GuiControls()
{
	if (ImGui::CollapsingHeader("Body")) {
		ImGui::AutoIndent indent;
		qvr::GuiControls(m_PhysicsComponent.GetBody());
	}
}

}