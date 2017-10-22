#pragma once

#include <json.hpp>

namespace qvr {

struct RenderSettings
{
	float m_RayLength = 50.0f;

	RenderSettings() = default;

	RenderSettings(const nlohmann::json& j) noexcept {
		if (j.is_object()) {
			m_RayLength = j.value<float>("RayLength", 50.0f);
		}
	}

	nlohmann::json ToJson() const {
		return nlohmann::json{ {"RayLength", m_RayLength} };
	}
};

}