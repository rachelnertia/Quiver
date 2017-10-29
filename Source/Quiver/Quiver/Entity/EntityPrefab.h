#pragma once

#include <vector>
#include <unordered_map>

#include <json.hpp>
#include <optional.hpp>

namespace qvr
{

class CustomComponentTypeLibrary;
class Entity;

class EntityPrefabContainer
{
public:
	const std::vector<std::string> GetPrefabNames() const;

	bool AddPrefab(
		const std::string prefabName,
		const Entity& entity,
		const CustomComponentTypeLibrary& customComponentTypes);

	const std::experimental::optional<nlohmann::json> GetPrefab(std::string prefabName) const;

	bool FromJson(const nlohmann::json& j);
	bool ToJson(nlohmann::json& j) const;

private:

	std::unordered_map<std::string, nlohmann::json> mEntityPrefabs;

};

}