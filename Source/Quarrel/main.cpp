#include "Quiver/Application/Application.h"
#include "Quiver/Application/ApplicationStateLibrary.h"
#include "Quiver/Application/Config.h"
#include "Quiver/Entity/CustomComponent/CustomComponent.h"

#include "Enemy/Enemy.h"
#include "Enemy/EnemyMelee.h"
#include "LevelSelect/LevelSelectFactory.h"
#include "Misc/Utils.h"
#include "Player/Player.h"
#include "Wanderer/Wanderer.h"
#include "WorldExit/WorldExit.h"

qvr::CustomComponentTypeLibrary CreateQuarrelTypes()
{
	using namespace qvr;

	CustomComponentTypeLibrary library;

	library.RegisterType(
		std::make_unique<CustomComponentType>(
			// type name
			"Player",
			// factory function
			[](Entity& entity) { return std::make_unique<Player>(entity); }));

	library.RegisterType(
		std::make_unique<CustomComponentType>(
			"Wanderer",
			[](Entity& entity) { return std::make_unique<Wanderer>(entity); }));

	library.RegisterType(
		std::make_unique<CustomComponentType>(
			"WorldExit",
			&CreateWorldExit));

	library.RegisterType(
		std::make_unique<CustomComponentType>(
			"Enemy",
			&CreateEnemy));

	library.RegisterType(
		std::make_unique<CustomComponentType>(
			"EnemyMelee",
			&CreateEnemyMelee));

	return library;
}

qvr::ApplicationStateLibrary CreateQuarrelApplicationStates()
{
	qvr::ApplicationStateLibrary library;

	library.AddStateCreator("LevelSelect", CreateLevelSelectState);

	return library;
}

int main()
{
	qvr::CustomComponentTypeLibrary quarrelTypes = CreateQuarrelTypes();
	qvr::ApplicationStateLibrary applicationStates = CreateQuarrelApplicationStates();
	qvr::FixtureFilterBitNames quarrelFilterBitNames = CreateFilterBitNames();
	qvr::ApplicationConfig qvrConfig = qvr::LoadConfig("config.json");

	qvr::ApplicationParams params{
		quarrelTypes,
		quarrelFilterBitNames,
		qvrConfig,
		applicationStates
	};

	return qvr::RunApplication(params);
}