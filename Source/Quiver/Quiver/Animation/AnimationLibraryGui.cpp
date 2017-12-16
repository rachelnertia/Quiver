#include "AnimationLibraryGui.h"

#include <ImGui/imgui.h>
#include <spdlog/fmt/fmt.h>
#include <spdlog/fmt/ostr.h>

#include "Quiver/Animation/AnimationLibrary.h"
#include "Quiver/Misc/ImGuiHelpers.h"
#include "Quiver/Misc/JsonHelpers.h"
#include "Quiver/Misc/Logging.h"

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

AnimationId AddAnimationFromJson(
	AnimationLibrary& library,
	const nlohmann::json& j,
	const AnimationSourceInfo& sourceInfo)
{
	auto log = GetConsoleLogger();
	static const char* logCtx = "AddAnimationFromJson:";

	// TODO: Restructure for early-out.

	if (const auto anim = AnimationData::FromJson(j)) {
		if ((*anim).IsValid()) {
			auto ret = library.Add(*anim, sourceInfo);
			if (ret != AnimationId::Invalid) {
				log->debug(
					"{} Successfully added animation from JSON to the library with ID {}",
					logCtx,
					ret);
				return GenerateAnimationId(*anim);
			}
			else {
				log->error(
					"{} Could not add animation data from JSON to the library",
					logCtx);
				return AnimationId::Invalid;
			}
		}
		else {
			log->error(
				"{} Animation data from JSON is not valid",
				logCtx);
			return AnimationId::Invalid;
		}
	}
	else {
		log->error(
			"{} Could not deserialize animation from JSON",
			logCtx);
		return AnimationId::Invalid;
	}
}

void AddAnimations(AnimationLibrary& library, AnimationLibraryEditorData& editorData)
{
	using namespace JsonHelp;

	ImGui::InputText("JSON Filename", editorData.mFilenameBuffer);

	if (ImGui::Button("Load Individual") &&
		strlen(editorData.mFilenameBuffer))
	{
		const nlohmann::json j = LoadJsonFromFile(editorData.mFilenameBuffer);

		AddAnimationFromJson(
			library,
			j,
			AnimationSourceInfo{ {}, editorData.mFilenameBuffer });
	}

	if (ImGui::Button("Load Collection") &&
		strlen(editorData.mFilenameBuffer))
	{
		const nlohmann::json j = LoadJsonFromFile(editorData.mFilenameBuffer);

		auto animCollection = j.get<std::map<std::string, nlohmann::json>>();

		for (auto& kvp : animCollection)
		{
			AddAnimationFromJson(
				library,
				kvp.second,
				AnimationSourceInfo{ kvp.first, editorData.mFilenameBuffer });
		}
	}
}

}