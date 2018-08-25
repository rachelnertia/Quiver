#pragma once

#include <json.hpp>

#include "PlayerQuiver.h"

// Stores all the quarrels that the player can put in their quiver.
struct PlayerQuarrelLibrary {
	using QuarrelList = std::vector<QuarrelTypeInfo>;
	QuarrelList quarrels;
};

inline void to_json(nlohmann::json& j, PlayerQuarrelLibrary const& library) {
	j = library.quarrels;
}

inline void from_json(nlohmann::json const& j, PlayerQuarrelLibrary& library) {
	if (j.is_array()) {
		library.quarrels = j.get<PlayerQuarrelLibrary::QuarrelList>();
	}
}