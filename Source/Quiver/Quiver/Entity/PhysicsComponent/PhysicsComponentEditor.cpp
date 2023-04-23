#include "PhysicsComponentEditor.h"

#include <Box2D/Dynamics/b2Body.h>
#include <Box2D/Dynamics/b2Fixture.h>
#include <ImGui/imgui.h>
#include <spdlog/fmt/fmt.h>

#include "Quiver/Entity/PhysicsComponent/PhysicsComponent.h"
#include "Quiver/Misc/ImGuiHelpers.h"

namespace qvr
{

using BitNames = FixtureFilterBitNames;

bool GuiControls(b2Filter& filter, const BitNames& bitNames)
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

	const int NumReservedFlags = 6;
	const int NumAvailableFlags = 16 - NumReservedFlags;

	if (ImGui::CollapsingHeader("Category Bits")) {
		ImGui::AutoIndent indent;
	
		for (int i = 0; i < NumAvailableFlags; i++) {
			modified |= ImGui::CheckboxFlags(
				fmt::format("{} - {} ({})", i, bitNames[i] ? bitNames[i] : "", 1 << i).c_str(), 
				(unsigned*)&filter.categoryBits, 
				1 << i);
		}
	}

	if (ImGui::CollapsingHeader("Mask Bits")) {
		ImGui::AutoIndent indent;

		for (int i = 0; i < NumAvailableFlags; i++) {
			modified |= ImGui::CheckboxFlags(
				fmt::format("{} - {} ({})", i, bitNames[i] ? bitNames[i] : "", 1 << i).c_str(),
				(unsigned*)&filter.maskBits,
				1 << i);
		}
	}

	return modified;
}

void GuiControls(b2Fixture& fixture, const BitNames& bitNames)
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

	ImGui::Checkbox(
		"Is Sensor",
		fixture,
		&b2Fixture::IsSensor,
		&b2Fixture::SetSensor);

	if (ImGui::CollapsingHeader("Filter Data"))
	{
		ImGui::AutoIndent indent;

		b2Filter filter = fixture.GetFilterData();

		if (GuiControls(filter, bitNames)) {
			fixture.SetFilterData(filter);
		}
	}
}

void GuiControls(b2Body& body, const BitNames& bitNames)
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
		ImGui::AutoID id(fixture);
		if (ImGui::CollapsingHeader("Fixture")) {
			ImGui::AutoIndent indent;
			GuiControls(*fixture, bitNames);
		}
		fixture = fixture->GetNext();
	}
}

PhysicsComponentEditor::PhysicsComponentEditor(PhysicsComponent& physicsComponent)
	: m_PhysicsComponent(physicsComponent)
{}

PhysicsComponentEditor::~PhysicsComponentEditor() {}

void PhysicsComponentEditor::GuiControls(const BitNames& bitNames)
{
	qvr::GuiControls(
		m_PhysicsComponent.GetBody(), 
		bitNames);

	ImGui::InputFloat(
		"Ground Offset (Bottom)",
		m_PhysicsComponent,
		&PhysicsComponent::GetBottom,
		&PhysicsComponent::SetBottom);
}

}