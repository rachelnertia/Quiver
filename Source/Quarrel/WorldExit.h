#pragma once

#include <memory>
#include <json.hpp>

namespace qvr {
class Entity;
class CustomComponent;
}

std::unique_ptr<qvr::CustomComponent> CreateWorldExit(qvr::Entity& entity);

bool VerifyWorldExitJson(const nlohmann::json& j);