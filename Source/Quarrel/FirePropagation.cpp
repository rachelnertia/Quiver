#include "FirePropagation.h"

#include "Effects.h"
#include "Misc/Utils.h"

void OnBeginContact(FiresInContact & fires, const b2Fixture & fixture)
{
	if (!FlagsAreSet(FixtureFilterCategories::Fire, GetCategoryBits(fixture))) {
		return;
	}

	auto foundIt =
		std::find(
			std::begin(fires.container),
			std::end(fires.container),
			&fixture);

	if (foundIt != std::end(fires.container)) {
		return;
	}

	fires.container.push_back(&fixture);
}

void OnEndContact(FiresInContact & fires, const b2Fixture & fixture)
{
	auto it = std::find(
		std::begin(fires.container),
		std::end(fires.container),
		&fixture);

	if (it != std::end(fires.container)) {
		fires.container.erase(it);
	}
}

void ApplyFires(FiresInContact & fires, ActiveEffectSet & effects)
{
	if (fires.container.empty()) return;

	AddActiveEffect(+ActiveEffectType::Burning, effects);
}
