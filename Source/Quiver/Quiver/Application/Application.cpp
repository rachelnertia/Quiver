#include "Application.h"

#include <iostream>
#include <fstream>
#include <memory>

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Event.hpp>

#include <ImGui/imgui.h>
#include <ImGui/imgui-SFML.h>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/bundled/time.h>
#include <cxxopts/cxxopts.hpp>
#include <optional.hpp>

#include "Quiver/Entity/CustomComponent/CustomComponent.h"
#include "Quiver/Misc/JsonHelpers.h"
#include "Quiver/Misc/Logging.h"
#include "Quiver/World/World.h"

#include "Game/Game.h"
#include "MainMenu/MainMenu.h"
#include "WorldEditor/WorldEditor.h"

namespace qvr {

using json = nlohmann::json;

json GetConfig();

std::unique_ptr<ApplicationState> GetInitialState(
	const json& config, 
	ApplicationStateContext& ctx);

struct WindowConfig {
	int width = 1024;
	int height = 576;
	std::string name = "Quiver Window";

	static WindowConfig Default;
};

WindowConfig WindowConfig::Default;

void from_json(const json& j, WindowConfig& config) {
	config.width = JsonHelp::GetValue(j, "width", WindowConfig::Default.width);
	config.height = JsonHelp::GetValue(j, "height", WindowConfig::Default.height);
	config.name = JsonHelp::GetValue(j, "name", WindowConfig::Default.name);
}

struct ImGuiConfig {
	float FontGlobalScale = 1.0f;
};

void from_json(const json& j, ImGuiConfig& config) {
	config.FontGlobalScale = JsonHelp::GetValue<float>(j, "FontGlobalScale", 1.0f);
}

void ConfigureImGui(const ImGuiConfig& config) {
	ImGuiIO& io = ImGui::GetIO();
	io.FontGlobalScale = config.FontGlobalScale;
}

void CreateSFMLWindow(sf::RenderWindow& window, const WindowConfig& config) {
	window.create(sf::VideoMode(config.width, config.height), config.name);
	// Key repeat is enabled by default, which is silly.
	window.setKeyRepeatEnabled(false);
}

int RunApplication(CustomComponentTypeLibrary& customComponentTypes)
{
	InitLoggers(spdlog::level::debug);

	auto consoleLog = spdlog::get("console");

	const json config = GetConfig();

	// Open Window
	sf::RenderWindow window;
	CreateSFMLWindow(
		window, 
		JsonHelp::GetValue(config, "windowConfig", WindowConfig::Default));

	ImGui::SFML::Init(window);
	
	ConfigureImGui(JsonHelp::GetValue(config, "ImGuiConfig", ImGuiConfig()));

	ApplicationStateContext applicationStateContext(
		window,
		customComponentTypes);

	consoleLog->info("Ready.");

	auto currentState = GetInitialState(config, applicationStateContext);

	// Main loop
	sf::Clock deltaClock;
	bool quit = false;
	while (!quit) {
		bool takeScreenshot = false;

		sf::Event windowEvent;
		while (window.pollEvent(windowEvent)) {
			ImGui::SFML::ProcessEvent(windowEvent);

			switch (windowEvent.type) {

			case sf::Event::Closed:
				quit = true;
				break;

			case sf::Event::KeyPressed:
				if (windowEvent.key.code == sf::Keyboard::SemiColon) {
					takeScreenshot = true;
				}
				break;

			case sf::Event::KeyReleased:
				break;

			case sf::Event::Resized:
				// The view doesn't do this on its own so everything goes weird.
				window.setView(
					sf::View(
						sf::FloatRect(
							0.0f,
							0.0f,
							(float)windowEvent.size.width,
							(float)windowEvent.size.height)));

				applicationStateContext.mWindowResized = true;

				break;

			case sf::Event::LostFocus:
				consoleLog->debug("Lost focus.");
				break;

			case sf::Event::GainedFocus:
				consoleLog->debug("Gained focus.");
				break;
			}
		}

		if (quit) continue;

		ImGui::SFML::Update(deltaClock.restart());

		currentState->ProcessFrame();

		if (takeScreenshot) {
			sf::Texture tex;
			tex.create(window.getSize().x, window.getSize().y);
			tex.update(window);
			sf::Image image = tex.copyToImage();
			
			// Save to screenshots/latest.png
			{
				const std::string latestPath = "screenshots/latest.png";

				// TODO: Detect if a 'screenshots' folder exists and, if it doesn't, create it.

				if (image.saveToFile(latestPath)) {
					consoleLog->info("Saved screenshot to {}", latestPath);
				}
				else {
					consoleLog->error("Failed to save screenshot to {}", latestPath);
				}
			}

			// Save with the date and time, down to the second.
			// E.g. screenshots/2017-12-10-20-25-3.png
			{
				const std::time_t now = std::time(nullptr);
				const std::string timePath = fmt::format("screenshots/{:%F-%H-%M-%S}.png", *std::localtime(&now));

				if (image.saveToFile(timePath)) {
					consoleLog->info("Saved screenshot to {}", timePath);
				}
				else {
					consoleLog->error("Failed to save screenshot to {}", timePath);
				}
			}
		}

		if (currentState->GetQuit())
		{
			currentState.reset(currentState->GetNextState().release());

			if (!currentState)
			{
				quit = true;
			}
		}

		applicationStateContext.mWindowResized = false;
	}

	consoleLog->info("Exiting.");

	ImGui::SFML::Shutdown();

	return 0;
}

json GetConfig()
{
	auto log = spdlog::get("console");
	assert(log.get() != nullptr);

	const char* configFileName = "config.json";

	log->info("Attempting to load configuration variables from {}...", configFileName);

	std::ifstream configFile(configFileName);

	if (!configFile.is_open()) {
		log->info(
			"Couldn't load {}! Will just go with all the hard-coded defaults.",
			configFileName);

		return {};
	}

	json config;

	try {
		config << configFile;
	}
	catch (std::invalid_argument e) {
		log->error(
			"Oh dear! There was an error parsing {}. "
			"Will just go with all the hard-coded defaults.",
			configFileName);

		return {};
	}

	log->info("Loaded configuration parameters from {}!", configFileName);

	log->debug("{}:\n{}", configFileName, config.dump(4));

	return config;
}

std::unique_ptr<ApplicationState> GetInitialState(
	const nlohmann::json& config,
	ApplicationStateContext& applicationStateContext)
{
	auto consoleLog = spdlog::get("console");
	assert(consoleLog.get() != nullptr);

	const auto initialState = JsonHelp::GetValue<std::string>(config, "initialState", {});

	if (initialState == "Editor")
	{
		consoleLog->info("Launching World Editor...");

		const auto worldFile = JsonHelp::GetValue<std::string>(config, "worldFile", {});

		if (!worldFile.empty()) {
			return std::make_unique<WorldEditor>(
				applicationStateContext,
				LoadWorld(worldFile, applicationStateContext.GetWorldContext()));
		}

		return std::make_unique<WorldEditor>(applicationStateContext);
	}
	else if (initialState == "Game")
	{
		consoleLog->info("Launching Game...");

		const auto worldFile = JsonHelp::GetValue<std::string>(config, "worldFile", {});

		if (!worldFile.empty()) {
			return std::make_unique<Game>(
				applicationStateContext,
				LoadWorld(worldFile, applicationStateContext.GetWorldContext()));
		}

		return std::make_unique<Game>(applicationStateContext);
	}
	else if (!initialState.empty())
	{
		consoleLog->warn(
			"'{}' is an unrecognised value for imguiConfig variable 'initialState'.", 
			initialState);
		consoleLog->info("Options are 'Editor' and 'Game'.");
	}

	consoleLog->info("Launching Main Menu...");

	return std::make_unique<MainMenu>(applicationStateContext);
}

int RunApplication() {
	CustomComponentTypeLibrary empty;
	return RunApplication(empty);
}

}