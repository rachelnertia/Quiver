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

#include "Quiver/Entity/CustomComponent/CustomComponent.h"
#include "Quiver/Misc/JsonHelpers.h"
#include "Quiver/Misc/Logging.h"
#include "Quiver/Physics/PhysicsUtils.h"
#include "Quiver/World/World.h"

#include "ApplicationStateLibrary.h"
#include "Config.h"
#include "Game/Game.h"
#include "MainMenu/MainMenu.h"
#include "WorldEditor/WorldEditor.h"

namespace qvr {

using json = nlohmann::json;

std::unique_ptr<ApplicationState> GetInitialState(
	const InitialStateConfig& config, 
	ApplicationStateContext& ctx,
	ApplicationStateLibrary& availableStates);

void ConfigureImGui(const ImGuiConfig& config) {
	ImGuiIO& io = ImGui::GetIO();
	io.FontGlobalScale = config.FontGlobalScale;
	io.FontAllowUserScaling = config.FontAllowUserScaling;
}

void CreateSFMLWindow(sf::RenderWindow& window, const WindowConfig& config) {
	window.create(sf::VideoMode(config.width, config.height), config.name);
	// Key repeat is enabled by default, which is silly.
	window.setKeyRepeatEnabled(false);
}

void TakeScreenshot(const sf::Window& window)
{
	auto consoleLog = GetConsoleLogger();

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

auto GetWorldFromParams(
	const json& stateParameters,
	WorldContext& worldContext)
	-> std::unique_ptr<World>
{
	if (stateParameters.empty()) {
		return nullptr;
	}

	const auto filename = stateParameters.value<std::string>("worldFile", {});

	if (filename.empty()) {
		return nullptr;
	}

	return LoadWorld(filename, worldContext);
};

auto CreateEditor(ApplicationStateContext& context, const json& params) 
-> std::unique_ptr<ApplicationState>
{
	auto world = GetWorldFromParams(
		params,
		context.GetWorldContext());

	if (world) {
		return std::make_unique<WorldEditor>(
			context,
			std::move(world));
	}

	return std::make_unique<WorldEditor>(context);
}

auto CreateGame(ApplicationStateContext& context, const json& params)
-> std::unique_ptr<ApplicationState>
{
	auto world = GetWorldFromParams(
		params,
		context.GetWorldContext());

	if (world) {
		return std::make_unique<Game>(
			context,
			std::move(world));
	}

	return std::make_unique<Game>(context);
}

ApplicationStateLibrary GetQuiverStates() {
	ApplicationStateLibrary lib;

	lib.AddStateCreator(
		"MainMenu",
		[](ApplicationStateContext& ctx, const json&) {
			return std::make_unique<MainMenu>(ctx);
		});
	
	lib.AddStateCreator("Editor", CreateEditor);
	lib.AddStateCreator("Game", CreateGame);

	return lib;
}

int RunApplication(ApplicationParams params)
{
	InitLoggers(spdlog::level::debug);

	auto consoleLog = spdlog::get("console");

	// Open Window
	sf::RenderWindow window;
	CreateSFMLWindow(window, params.config.windowConfig);

	ImGui::SFML::Init(window);
	
	ConfigureImGui(params.config.imGuiConfig);

	ApplicationStateContext applicationStateContext(
		window,
		params.customComponentTypes,
		params.fixtureFilterBitNames,
		params.config.graphicsSettings);

	consoleLog->info("Ready.");

	auto currentState =
		params.userStates.CreateState(
			params.config.initialState.stateName.c_str(),
			params.config.initialState.stateParameters,
			applicationStateContext);

	if (currentState == nullptr) {
		ApplicationStateLibrary quiverStates = GetQuiverStates();
		currentState = GetInitialState(
			params.config.initialState,
			applicationStateContext,
			quiverStates);
	}

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

		ImGui::SFML::Update(window, deltaClock.restart());

		currentState->ProcessFrame();

		if (takeScreenshot) {
			TakeScreenshot(window);
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


void from_json(const json& j, WindowConfig& config) {
	config.width = j.value("width", config.width);
	config.height = j.value("height", config.height);
	config.name = j.value("name", config.name);
}

void from_json(const json& j, ImGuiConfig& config) {
	config.FontGlobalScale = j.value("FontGlobalScale", config.FontGlobalScale);
	config.FontAllowUserScaling = j.value("FontAllowUserScaling", config.FontAllowUserScaling);
}

void from_json(const nlohmann::json& j, GraphicsSettings& g) {
	if (!j.is_object()) return;

	g.frameTextureResolutionRatio = j.value("frameTextureResolutionRatio", 1.0f);
}

void from_json(const json& j, InitialStateConfig& config) {
	config.stateName = j.value("name", config.stateName);
	config.stateParameters = j.value("params", config.stateParameters);
}

template <typename T, std::size_t N>
constexpr std::size_t countof(T const (&)[N]) noexcept
{
	return N;
}

bool GetLogLevel(const json& j, spdlog::level::level_enum& level) {
	using namespace spdlog::level;
	
	if (j.is_number_unsigned()) {
		const unsigned u = j.get<unsigned>();

		if (u > (unsigned)level_enum::off) {
			return false;
		}

		return (level_enum)u;
	}

	if (j.is_string()) {
		const auto s = j.get<std::string>();

		for (unsigned index = 0; index < countof(level_names); index++) {
			if (level_names[index] == s) {
				level = (level_enum)index;
				return true;
			}

			if (short_level_names[index] == s) {
				level = (level_enum)index;
				return true;
			}
		}
	}

	return false;
}

void from_json(const json& j, LoggingConfig config) {
	GetLogLevel(j.value<json>("level", {}), config.level);
}

void from_json(const json& j, ApplicationConfig& config) {
	config.windowConfig = j.value("windowConfig", config.windowConfig);
	config.imGuiConfig = j.value("ImGuiConfig", config.imGuiConfig);
	config.graphicsSettings = j.value("graphicsSettings", config.graphicsSettings);
	config.initialState = j.value("initialState", config.initialState);
}

ApplicationConfig LoadConfig(const char* filename) {
	InitLoggers(spdlog::level::debug);
	
	auto log = spdlog::get("console");
	assert(log.get() != nullptr);

	log->info("Attempting to load configuration variables from {}...", filename);

	std::ifstream configFile(filename);
	
	if (!configFile.is_open()) {
		log->info(
			"Couldn't load {}! Will just go with all the hard-coded defaults.",
			filename);

		return ApplicationConfig();
	}

	json configJson;

	try {
		configFile >> configJson;
	}
	catch (std::invalid_argument e) {
		log->error(
			"Oh dear! There was an error parsing {} as JSON. "
			"Will just go with all the hard-coded defaults.",
			filename);

		return ApplicationConfig();
	}

	log->info("Loaded configuration parameters from {}!", filename);

	log->debug("{}:\n{}", filename, configJson.dump(4));

	ApplicationConfig config = configJson;

	return config;
}

std::unique_ptr<ApplicationState> GetInitialState(
	const InitialStateConfig& config,
	ApplicationStateContext& applicationStateContext,
	ApplicationStateLibrary& stateFactories)
{
	auto consoleLog = spdlog::get("console");
	assert(consoleLog.get() != nullptr);

	consoleLog->info("Launching {}...", config.stateName);

	auto newState =
		stateFactories.CreateState(
			config.stateName.c_str(),
			config.stateParameters,
			applicationStateContext);

	if (!newState)
	{
		consoleLog->warn(
			"Failed to launch application state '{}'", 
			config.stateName);
		consoleLog->info("Launching Main Menu...");
		return std::make_unique<MainMenu>(applicationStateContext);
	}

	return newState;
}

int RunApplication(
	CustomComponentTypeLibrary& customComponentTypes,
	FixtureFilterBitNames& fixtureFilterBitNames)
{
	ApplicationStateLibrary noUserStates;
	ApplicationConfig config;
	return RunApplication(
		ApplicationParams{
			customComponentTypes,
			fixtureFilterBitNames,
			config,
			noUserStates
		});
}

int RunApplication(CustomComponentTypeLibrary& customComponents)
{
	FixtureFilterBitNames bitNames{};
	return RunApplication(customComponents, bitNames);
}

int RunApplication() {
	CustomComponentTypeLibrary empty;
	return RunApplication(empty);
}

bool ApplicationStateLibrary::AddStateCreator(std::string name, Factory factory) 
{
	if (map.count(name) != 0) {
		return false;
	}

	map[name] = factory;
	return true;
}

auto ApplicationStateLibrary::CreateState(
	const char* stateName,
	const nlohmann::json& stateParams,
	ApplicationStateContext& context)
		-> std::unique_ptr<ApplicationState>
{
	if (map.count(stateName) == 0) {
		return nullptr;
	}

	const auto& factory = map.at(stateName);
	
	return factory(context, stateParams);
}

}