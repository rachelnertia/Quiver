#include "RenderComponentEditor.h"

#include <ImGui/imgui.h>
#include <ImGui/imgui-SFML.h>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

#include "Quiver/Animation/AnimationSystem.h"
#include "Quiver/Animation/AnimationLibraryGui.h"
#include "Quiver/Entity/Entity.h"
#include "Quiver/Entity/RenderComponent/RenderComponent.h"
#include "Quiver/Graphics/ColourUtils.h"
#include "Quiver/Misc/ImGuiHelpers.h"
#include "Quiver/World/World.h"

namespace
{

using namespace qvr;

AnimationSystem& GetAnimationSystem(const RenderComponent& renderComponent) {
	return renderComponent.GetEntity().GetWorld().GetAnimationSystem();
}

}

namespace qvr
{

RenderComponentEditor::RenderComponentEditor(RenderComponent& renderComponent)
	: m_RenderComponent(renderComponent)
{}

RenderComponentEditor::~RenderComponentEditor() {}

void RenderComponentEditor::GuiControls()
{
	{
		bool detached = m_RenderComponent.IsDetached();
		if (ImGui::Checkbox("Detached", &detached)) {
			m_RenderComponent.SetDetached(detached);
		}
	}

	{
		float height = m_RenderComponent.GetHeight();
		if (ImGui::SliderFloat("Height", &height, 0.5f, 8.0f)) {
			m_RenderComponent.SetHeight(height);
		}
	}

	{
		float groundOffset = m_RenderComponent.GetGroundOffset();
		if (ImGui::SliderFloat("Ground Offset", &groundOffset, 0.0f, 5.0f)) {
			m_RenderComponent.SetGroundOffset(groundOffset);
		}
	}

	{
		float spriteRadius = m_RenderComponent.GetSpriteRadius();
		if (ImGui::SliderFloat("Sprite Radius", &spriteRadius, 0.1f, 2.0f)) {
			m_RenderComponent.SetSpriteRadius(spriteRadius);
		}
	}

	{
		sf::Color c = m_RenderComponent.GetColor();
		ColourUtils::ImGuiColourEdit("Colour", c);
		m_RenderComponent.SetColor(c);
	}

	if (ImGui::CollapsingHeader("Texture")) {
		ImGui::AutoIndent indent;

		if (m_RenderComponent.GetTexture() != nullptr) {
			ImGui::Text("Filename: %s", m_RenderComponent.GetTextureFilename());
			
			if (ImGui::Button("Remove Texture")) {
				m_RenderComponent.RemoveTexture();
			}
			else {
				ImGui::Image(*m_RenderComponent.GetTexture());

				const Animation::Rect rect = m_RenderComponent.GetTextureRect();

				ImGui::Image(*m_RenderComponent.GetTexture(),
					sf::FloatRect(
						(float)rect.left,
						(float)rect.top,
						(float)rect.right - rect.left,
						(float)rect.bottom - rect.top));
			}
		}
		else {
			ImGui::Text("No Texture");
			
			ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue;
			
			std::string filename;

			if (ImGui::InputText<128>("Filename", filename, flags) ||			
				ImGui::Button("Try to Load")) 
			{
				if (!filename.empty()) {
					m_RenderComponent.SetTexture(filename);
				}
			}
		}
	}

	if (ImGui::CollapsingHeader("Animation##RC")) {
		ImGui::AutoIndent indent;

		AnimationSystem& animSystem = GetAnimationSystem(m_RenderComponent);

		if (ImGui::CollapsingHeader("Pick Animation")) {
			int selection = -1;
			const AnimationId animation = PickAnimation(animSystem.GetAnimations(), selection);

			if (animation != AnimationId::Invalid) {
				m_RenderComponent.SetAnimation(animation);
			}
		}

		if (animSystem.AnimatorExists(m_RenderComponent.GetAnimatorId())) {
			ImGui::Text("Animator ID:\t%u", m_RenderComponent.GetAnimatorId());
			animSystem.AnimatorGui(m_RenderComponent.GetAnimatorId());

			if (ImGui::Button("Remove Animator")) {
				m_RenderComponent.RemoveAnimation();
			}
		}
		else {
			ImGui::Text("No Animator");

			if (ImGui::CollapsingHeader("Set Texture Rect")) {
				ImGui::AutoIndent indent2;

				Animation::Rect rect = m_RenderComponent.GetTextureRect();

				int corner[2] = { rect.left, rect.top };
				if (ImGui::InputInt2("Top Left (X, Y)", corner)) {
					rect.left = corner[0];
					rect.top = corner[1];
				}
				corner[0] = rect.right;
				corner[1] = rect.bottom;
				if (ImGui::InputInt2("Bottom Right (X, Y)", corner)) {
					rect.right = corner[0];
					rect.bottom = corner[1];
				}

				m_RenderComponent.SetTextureRect(rect);
			}
		}
	}
}

}