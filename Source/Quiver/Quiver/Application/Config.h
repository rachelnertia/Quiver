#pragma once

#include <json.hpp>
#include <spdlog/common.h>

#include "GraphicsSettings.h"

namespace qvr {
struct WindowConfig {
	int width = 1024;
	int height = 576;
	std::string name = "Quiver Window";
};

struct ImGuiConfig {
	float FontGlobalScale = 1.0f;
	bool FontAllowUserScaling = false;
};

struct InitialStateConfig {
	std::string stateName = "Editor";
	nlohmann::json stateParameters;
};

struct LoggingConfig {
	spdlog::level::level_enum level = spdlog::level::debug;
};

struct ApplicationConfig {
	WindowConfig windowConfig;
	ImGuiConfig imGuiConfig;
	GraphicsSettings graphicsSettings;
	InitialStateConfig initialState;
	LoggingConfig logging;
};

ApplicationConfig LoadConfig(const char* filename);

}