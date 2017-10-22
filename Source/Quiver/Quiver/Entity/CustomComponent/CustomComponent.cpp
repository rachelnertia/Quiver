#include "CustomComponent.h"

#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>

#include "Quiver/Entity/Entity.h"
#include "Quiver/World/World.h"

namespace qvr {

CustomComponentType::CustomComponentType(
	const std::string typeName,
	std::function<std::unique_ptr<CustomComponent>(Entity&)> factoryFunc,
	std::function<bool(const nlohmann::json&)> verifyJsonFunc)
	: mName(typeName),
	mFactoryFunc(factoryFunc),
	mVerifyJsonFunc(verifyJsonFunc)
{}

std::unique_ptr<CustomComponent>
CustomComponentType::CreateInstance(
	Entity & entity,
	const nlohmann::json & j)
{
	if (!VerifyJson(j)) {
		return nullptr;
	}

	auto instance = mFactoryFunc(entity);

	if (instance) {
		if (!instance->FromJson(j)) {
			return nullptr;
		}
	}

	return instance;
}

bool CustomComponentTypeLibrary::RegisterType(
	std::unique_ptr<CustomComponentType> type)
{
	if (mTypes.find(type->GetName()) != mTypes.end()) {
		return false;
	}

	mTypes[type->GetName()] = std::move(type);

	return true;
}

bool CustomComponentTypeLibrary::ForgetType(
	const std::string typeName)
{
	const auto& it = mTypes.find(typeName);

	if (it == mTypes.end()) {
		return false;
	}

	mTypes.erase(typeName);

	return true;
}

bool CustomComponentTypeLibrary::TypeExists(const std::string typeName) const
{
	return (mTypes.find(typeName) != mTypes.end());
}

CustomComponentType* CustomComponentTypeLibrary::GetType(
	const std::string typeName) const
{
	const auto& it = mTypes.find(typeName);

	if (it == mTypes.end()) {
		return nullptr;
	}

	return it->second.get();
}

std::vector<std::string> CustomComponentTypeLibrary::GetTypeNames() const
{
	std::vector<std::string> v;
	for (const auto& kvp : mTypes) {
		v.push_back(kvp.first);
	}
	return v;
}

CustomComponent::CustomComponent(Entity& entity)
	: Component(entity)
{
	GetEntity().GetWorld().RegisterCustomComponent(*this);
}

CustomComponent::~CustomComponent()
{
	GetEntity().GetWorld().UnregisterCustomComponent(*this);
}

bool CustomComponentTypeLibrary::IsValid(const nlohmann::json& j) const
{
	auto log = spdlog::get("console");
	assert(log);
	const char* logCtx = "CustomComponent::VerifyJson";

	if (j.empty()) {
		log->error("{}: JSON is empty.");
		return false;
	}
	if (!j.is_object()) {
		log->error("{}: JSON is not an object.");
		return false;
	}
	if (j.find("Type") == j.end()) {
		log->error("{}: JSON has no 'Type' member.");
		return false;
	}
	if (!j["Type"].is_string()) {
		log->error("{}: 'Type' member is not a string.");
		return false;
	}

	const std::string typeName = j["Type"].get<std::string>();

	const CustomComponentType* type = GetType(typeName);

	if (!type) {
		log->error("{}: There is no CustomComponent Type with the name '{}'.", logCtx, typeName);
		return false;
	}

	if (j.count("Data") != 0) {
		if (!type->VerifyJson(j["Data"])) {
			log->error("{}: 'Data' field is not valid for CustomComponent of Type '{}'.", logCtx, typeName);
			return false;
		}
	}

	return true;
}

std::unique_ptr<CustomComponent>
CustomComponentTypeLibrary::CreateInstance(
	Entity& entity,
	const nlohmann::json & j) const
{
	auto log = spdlog::get("console");
	assert(log);
	const char* logCtx = "CustomComponentTypeLibrary::CreateInstance";

	if (j.find("Type") == j.end() ||
		!j["Type"].is_string())
	{
		log->error("{}: Could not find 'Type' field in JSON.", logCtx);
		return nullptr;
	}

	std::string typeName = j["Type"].get<std::string>();

	CustomComponentType* type = GetType(typeName);

	if (!type) {
		log->error("{}: '{}' could not be found in the Input Component Type Library", logCtx, typeName);
		return nullptr;
	}

	// Some CustomComponents override ToJson, and the json they output from that method
	// goes into the "Data" field.
	if (j.count("Data") != 0)
	{
		if (j["Data"].is_object())
		{
			return type->CreateInstance(entity, j["Data"]);
		}
	}

	return type->CreateInstance(entity);
}

}