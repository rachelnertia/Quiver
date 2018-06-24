#pragma once

#include <json.hpp>

#include "PlayerQuiver.h"

// Stores all the quarrels that the player can put in their quiver.
struct PlayerQuarrelLibrary {
	using QuarrelMap = std::unordered_map<std::string, QuarrelTypeInfo>;
	QuarrelMap quarrels;
};

inline void to_json(nlohmann::json& j, PlayerQuarrelLibrary const& library) {
	j = library.quarrels;
}

inline void from_json(nlohmann::json const& j, PlayerQuarrelLibrary& library) {
	library.quarrels = j.get<PlayerQuarrelLibrary::QuarrelMap>();
}