#include "Quiver/Application/Application.h"
#include "Quiver/Entity/CustomComponent/CustomComponent.h"

#include "Enemy.h"
#include "Player.h"
#include "Wanderer.h"
#include "WorldExit.h"

qvr::CustomComponentTypeLibrary CreateQuarrelTypes()
{
	using namespace qvr;

	CustomComponentTypeLibrary library;

	library.RegisterType(
		std::make_unique<CustomComponentType>(
			// type name
			"Player",
			// factory function
			[](Entity& entity) { return std::make_unique<Player>(entity); },
			// json verification function 
			&Player::VerifyJson));

	library.RegisterType(
		std::make_unique<CustomComponentType>(
			"Wanderer",
			[](Entity& entity) { return std::make_unique<Wanderer>(entity); },
			[](const nlohmann::json&) { return true; }));

	library.RegisterType(
		std::make_unique<CustomComponentType>(
			"WorldExit",
			&CreateWorldExit,
			&VerifyWorldExitJson));

	library.RegisterType(
		std::make_unique<CustomComponentType>(
			"Enemy",
			&CreateEnemy,
			[](const nlohmann::json&) { return true; }));

	return library;
}

int main()
{
	qvr::CustomComponentTypeLibrary quarrelTypes = CreateQuarrelTypes();

	return qvr::RunApplication(quarrelTypes);
}