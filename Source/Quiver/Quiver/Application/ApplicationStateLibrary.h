#pragma once

#include <unordered_map>

#include <function2.hpp>
#include <json.hpp>

namespace qvr {

class ApplicationState;
class ApplicationStateContext;

class ApplicationStateLibrary {
public:
	using FactoryReturnType = std::unique_ptr<ApplicationState>;
	using Factory = 
		fu2::function<FactoryReturnType(
			ApplicationStateContext& context, 
			const nlohmann::json&) const>;

	bool AddStateCreator(std::string name, Factory factory);

	auto CreateState(
		const char* stateName,
		const nlohmann::json& stateParams,
		ApplicationStateContext& context)
			-> std::unique_ptr<ApplicationState>;

private:
	using FactoryMap = std::unordered_map<std::string, Factory>;

	FactoryMap map;
};

}