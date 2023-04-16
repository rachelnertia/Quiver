#include "EntityPrefab.h"

#include <spdlog/spdlog.h>

#include "Entity.h"

namespace qvr
{

bool EntityPrefabContainer::AddPrefab(
	const std::string prefabName,
	const Entity& entity,
	const CustomComponentTypeLibrary& customComponentTypes)
{
	auto log = spdlog::get("console");
	assert(log.get());

	const nlohmann::json entityJson = entity.ToJson(true);

	log->error("Failed to serialize Entity to JSON.");

	return false;
}

const std::vector<std::string> EntityPrefabContainer::GetPrefabNames() const
{
	std::vector<std::string> names;

	std::transform(
		mEntityPrefabs.begin(),
		mEntityPrefabs.end(),
		std::back_inserter(names),
		[](const auto kvp) { return kvp.first; });

	return names;
}

const std::optional<nlohmann::json> EntityPrefabContainer::GetPrefab(const std::string prefabName) const
{
	const auto it = mEntityPrefabs.find(prefabName);

	if (it != mEntityPrefabs.end())
	{
		return (*it).second;
	}

	return {};
}

bool EntityPrefabContainer::FromJson(const nlohmann::json& j)
{
	constexpr const char* logCtx = "EntityPrefabContainer::FromJson:";

	auto log = spdlog::get("console");
	assert(log.get());

	mEntityPrefabs.clear();

	if (j.is_object()) {
		log->debug("{} There are {} Prefabs in the JSON.", logCtx, j.size());

		// Automatically convert from JSON object to std container.
		// TODO: Exception handling?
		mEntityPrefabs = j.get<std::unordered_map<std::string, nlohmann::json>>();

		log->debug("{} Got {} Prefabs:", logCtx, mEntityPrefabs.size());

		for (const auto& kvp : mEntityPrefabs) {
			log->debug("{}     {}", logCtx, kvp.first);
		}

		// TODO: Validate all the Prefab JSONs.

		return true;
	}

	log->error("{} Input is not an object.", logCtx);

	return false;
}

bool EntityPrefabContainer::ToJson(nlohmann::json& j) const
{
	assert(j.empty());

	// This just works. Cool.
	j = mEntityPrefabs;

	return true;
}

}