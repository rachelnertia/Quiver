#pragma once

#include <json.hpp>

namespace JsonHelp
{
	nlohmann::json LoadJsonFromFile(const std::string filename);

	// json::value throws if called on a nlohmann::json that isn't an object.
	// This protects us against that.
	// Also protects us against the exception thrown when the key is found but it is
	// not convertible to T.
	template <typename T>
	T GetValue(const nlohmann::json& j, const char* name, T defaultValue)
	{
		try {
			return j.value<T>(name, defaultValue);
		}
		catch (std::domain_error e)
		{
			return defaultValue;
		}
	}
}