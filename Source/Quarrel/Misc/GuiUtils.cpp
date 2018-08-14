#include "GuiUtils.h"

#include <ImGui/imgui.h>
#include <Quiver/Misc/ImGuiHelpers.h>
#include <spdlog/fmt/fmt.h>

#include <Quiver/Animation/Animators.h>
#include <Quiver/Animation/AnimationLibraryGui.h>

using namespace qvr;

std::experimental::optional<AnimationId> PickAnimationGui(
	const char* title,
	const AnimationId currentAnim,
	const AnimatorCollection& animators)
{
	if (ImGui::CollapsingHeader(title))
	{
		ImGui::AutoIndent autoIndent;

		if (currentAnim != AnimationId::Invalid)
		{
			if (auto animSourceInfo = animators.GetAnimations().GetSourceInfo(currentAnim))
			{
				ImGui::Text("Animation ID: %d", currentAnim.GetValue());
				ImGui::Text("Animation Data Source:");
				ImGui::Text("  Name: %s", animSourceInfo->name.c_str());
				ImGui::Text("  File: %s", animSourceInfo->filename.c_str());
			}

			if (ImGui::Button(fmt::format("No {}", title).c_str()))
			{
				return AnimationId::Invalid;
			}
		}
		else
		{
			int selection = -1;
			return PickAnimation(animators.GetAnimations(), selection);
		}
	}

	return {};
}
