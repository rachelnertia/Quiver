#include "JsonHelpers.h"

#include <fstream>

#include <spdlog/spdlog.h>

using json = nlohmann::json;

json JsonHelp::LoadJsonFromFile(const std::string filename)
{
	auto log = spdlog::get("console");
	assert(log.get() != nullptr);

	std::ifstream file(filename);

	if (!file.is_open())
	{
		log->error("Could not open file '{}'", filename);
		return json();
	}

	try
	{
		json j;
		file >> j;
		return j;
	}
	catch (std::invalid_argument exception)
	{
		log->error("Error parsing JSON from file! Exception: '{}'", exception.what());
	}

	return nlohmann::json();
}
