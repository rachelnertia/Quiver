#include "AnimationLibraryGui.h"

#include <ImGui/imgui.h>
#include <spdlog/fmt/fmt.h>
#include <spdlog/fmt/ostr.h>

#include "Quiver/Animation/AnimationLibrary.h"

namespace qvr
{

AnimationId PickAnimation(const AnimationLibrary& animations, int& currentSelection)
{
	if (animations.GetCount() == 0) {
		ImGui::Text("No Animations to display");
	}

	std::vector<AnimationId> animationIds = animations.GetIds();

	std::vector<std::string> animationIdStrings;

	std::transform(
		std::begin(animationIds),
		std::end(animationIds),
		std::back_inserter(animationIdStrings),
		[](const AnimationId id)
	{
		return fmt::format("ID: {}", id);
	});

	if (currentSelection > (int)(animations.GetCount()) - 1) {
		currentSelection = -1;
	}

	auto itemsGetter = [](void* data, int idx, const char** out_text) -> bool
	{
		auto container = static_cast<std::vector<std::string>*>(data);

		*out_text = (*container)[idx].c_str();

		return true;
	};

	ImGui::Combo(
		"List",
		&currentSelection,
		itemsGetter,
		(void*)&animationIdStrings,
		animationIdStrings.size());

	if (currentSelection > -1) {
		return animationIds[currentSelection];
	}

	return AnimationId::Invalid;
}

}