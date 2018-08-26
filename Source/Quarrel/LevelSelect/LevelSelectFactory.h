#pragma once

#include <memory>

#include <json.hpp>

#include <Quiver/Application/ApplicationState.h>


auto CreateLevelSelectState(qvr::ApplicationStateContext& context, const nlohmann::json& params)
	->std::unique_ptr<qvr::ApplicationState>;