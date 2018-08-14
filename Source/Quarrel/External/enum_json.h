#pragma once

#include <json.hpp>

// to_json and from_json templates for enums created with the BETTER_ENUM macro
// (see enum.h)

// Include this in files which implicitly call to/from_json with your BETTER_ENUM.

// I don't really know what's going on here. ADL? Or magic?

template <typename Enum>
void to_json(nlohmann::json & j, const Enum & e) {
	j = e._to_string();
}

template <typename Enum>
void from_json(const nlohmann::json & j, Enum & e) {
	if (j.is_string()) {
		auto str = j.get<std::string>();
		e = Enum::_from_string(str.c_str());
	}
	else
	{
		e = Enum::_from_integral(j.get<int>());
	}
}