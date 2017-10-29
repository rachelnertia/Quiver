#include "PhysicsComponentEditor.h"

#include <Box2D/Dynamics/b2Body.h>
#include <Box2D/Dynamics/b2Fixture.h>
#include <ImGui/imgui.h>

#include "Quiver/Entity/PhysicsComponent/PhysicsComponent.h"
#include "Quiver/Misc/ImGuiHelpers.h"

namespace qvr
{

void GuiControls(b2Fixture& fixture)
{
	{
		float friction = fixture.GetFriction();
		if (ImGui::SliderFloat("Friction", &friction, 0.0f, 1.0f)) {
			fixture.SetFriction(friction);
		}
	}

	{
		float restitution = fixture.GetRestitution();
		if (ImGui::SliderFloat("Restitution", &restitution, 0.0f, 2.0f)) {
			fixture.SetRestitution(restitution);
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

	{
		bool fixedRotation = body.IsFixedRotation();
		if (ImGui::Checkbox("Fixed Rotation", &fixedRotation)) {
			body.SetFixedRotation(fixedRotation);
		}
	}

	{
		float linearDamping = body.GetLinearDamping();
		if (ImGui::SliderFloat("Linear Damping", &linearDamping, 0.0f, 10.0f)) {
			body.SetLinearDamping(linearDamping);
		}
	}

	{
		float angularDamping = body.GetAngularDamping();
		if (ImGui::InputFloat("Angular Damping", &angularDamping)) {
			body.SetAngularDamping(angularDamping);
		}
	}

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
	qvr::GuiControls(m_PhysicsComponent.GetBody());
}

}