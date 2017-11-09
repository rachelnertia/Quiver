#include "Application.h"

#include <iostream>
#include <fstream>
#include <memory>

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Event.hpp>

#include <ImGui/imgui.h>
#include <ImGui/imgui-SFML.h>

#include <spdlog/spdlog.h>
#include <cxxopts/cxxopts.hpp>
#include <optional.hpp>

#include "Quiver/Entity/CustomComponent/CustomComponent.h"
#include "Quiver/World/World.h"

#include "Game/Game.h"
#include "MainMenu/MainMenu.h"
#include "WorldEditor/WorldEditor.h"

namespace qvr {

nlohmann::json GetConfig();

std::unique_ptr<ApplicationState> GetInitialState(
	const nlohmann::json& config, 
	ApplicationStateContext& ctx);

int RunApplication(CustomComponentTypeLibrary& customComponentTypes)
{
	// Console logger with colour.
	auto consoleLog = spdlog::stdout_color_mt("console");

	consoleLog->set_level(spdlog::level::debug); // Log everything
	consoleLog->set_pattern("[%l] %v");          // [level] msg

	const nlohmann::json config = GetConfig();

	// Open Window
	sf::RenderWindow window;
	window.create(sf::VideoMode(1024, 576), "SFML Window");

	// Set some window settings.
	// Key repeat is enabled by default, which is silly.
	window.setKeyRepeatEnabled(false);

	ImGui::SFML::Init(window);

	ApplicationStateContext applicationStateContext(
		window,
		customComponentTypes);

	consoleLog->info("Ready.");

	auto currentState = GetInitialState(config, applicationStateContext);

	// Main loop
	sf::Clock deltaClock;
	bool quit = false;
	while (!quit) {
		sf::Event windowEvent;
		while (window.pollEvent(windowEvent)) {
			ImGui::SFML::ProcessEvent(windowEvent);

			switch (windowEvent.type) {

			case sf::Event::Closed:
				quit = true;
				break;

			case sf::Event::KeyPressed:
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

		ImGui::SFML::Update(deltaClock.restart());

		currentState->ProcessFrame();

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

	window.close();

	spdlog::drop_all();

	return 0;
}

nlohmann::json GetConfig()
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

		return nlohmann::json();
	}

	nlohmann::json config;

	try {
		config << configFile;
	}
	catch (std::invalid_argument e) {
		log->error(
			"Oh dear! There was an error parsing {}. "
			"Will just go with all the hard-coded defaults.",
			configFileName);

		return nlohmann::json();
	}

	log->info("Loaded configuration parameters from {}!", configFileName);

	log->debug("{}:\n{}", configFileName, config.dump(4));

	return config;
}

template <typename T>
T GetValue(const nlohmann::json& j, const char* name, T defaultValue)
{
	if (!j.is_object())
	{
		return defaultValue;
	}

	return j.value<T>(name, defaultValue);
}

std::unique_ptr<ApplicationState> GetInitialState(
	const nlohmann::json& config,
	ApplicationStateContext& applicationStateContext)
{
	auto consoleLog = spdlog::get("console");
	assert(consoleLog.get() != nullptr);

	const auto initialState = GetValue<std::string>(config, "initialState", {});

	if (initialState == "Editor")
	{
		consoleLog->info("Launching World Editor...");

		const auto worldFile = GetValue<std::string>(config, "worldFile", {});

		if (!worldFile.empty()) {
			return std::make_unique<WorldEditor>(
				applicationStateContext,
				LoadWorld(worldFile, applicationStateContext.GetCustomComponentTypes()));
		}

		return std::make_unique<WorldEditor>(applicationStateContext);
	}
	else if (initialState == "Game")
	{
		consoleLog->info("Launching Game...");

		const auto worldFile = GetValue<std::string>(config, "worldFile", {});

		if (!worldFile.empty()) {
			return std::make_unique<Game>(
				applicationStateContext,
				LoadWorld(worldFile, applicationStateContext.GetCustomComponentTypes()));
		}

		return std::make_unique<Game>(applicationStateContext);
	}
	else if (!initialState.empty())
	{
		consoleLog->warn(
			"'{}' is an unrecognised value for config variable 'initialState'.", 
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