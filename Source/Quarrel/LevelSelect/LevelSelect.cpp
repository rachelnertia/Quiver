#include <json.hpp>

#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/RenderWindow.hpp>

#include <Quiver/Application/ApplicationState.h>
#include <Quiver/Misc/ImGuiHelpers.h>

using namespace qvr;
using json = nlohmann::json;

struct LevelDesc {
	std::string name;
	std::string path;
};

void from_json(const json& j, LevelDesc& desc) {
	desc.name = j.at("name").get<std::string>();
	desc.path = j.at("path").get<std::string>();
}

class LevelSelect : public ApplicationState {

	std::vector<LevelDesc> levels;

public:
	LevelSelect(ApplicationStateContext& ctx, std::vector<LevelDesc>&& levels) 
		: ApplicationState(ctx)
		, levels(std::move(levels))
	{}

	void ProcessFrame() override;
};

auto CreateLevelSelectState(qvr::ApplicationStateContext& context, const json& params)
	->std::unique_ptr<qvr::ApplicationState>
{
	std::vector<LevelDesc> levels;

	try {
		levels = params.value<std::vector<LevelDesc>>("levels", {});
	}
	catch (std::domain_error) {}
	catch (std::out_of_range) {}

	return std::make_unique<LevelSelect>(context, std::move(levels));
}

void LevelSelect::ProcessFrame() {
	{
		ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
		ImGui::SetNextWindowSize(ImVec2(600.0f, (float)GetContext().GetWindow().getSize().y));

		ImGui::AutoWindow window("Level Select", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

		ImGui::AutoStyleVar styleVar(ImGuiStyleVar_IndentSpacing, 5.0f);
	}

	{
		GetContext().GetWindow().clear(sf::Color::Black);

		ImGui::Render();

		GetContext().GetWindow().display();
	}
}