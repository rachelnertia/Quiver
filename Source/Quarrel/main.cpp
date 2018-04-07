#include "Quiver/Application/Application.h"
#include "Quiver/Entity/CustomComponent/CustomComponent.h"

#include "Enemy.h"
#include "EnemyMelee.h"
#include "Player.h"
#include "Wanderer.h"
#include "WorldExit.h"
#include "Utils.h"

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