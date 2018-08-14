#include "Quiver/Application/Application.h"
#include "Quiver/Entity/CustomComponent/CustomComponent.h"

#include "Enemy/Enemy.h"
#include "Enemy/EnemyMelee.h"
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

int main()
{
	qvr::CustomComponentTypeLibrary quarrelTypes = CreateQuarrelTypes();
	qvr::FixtureFilterBitNames quarrelFilterBitNames = CreateFilterBitNames();

	return qvr::RunApplication(quarrelTypes, quarrelFilterBitNames);
}