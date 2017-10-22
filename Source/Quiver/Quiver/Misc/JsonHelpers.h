#pragma once

#include <json.hpp>

namespace JsonHelp
{
	nlohmann::json LoadJsonFromFile(const std::string filename);
}