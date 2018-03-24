#include "CrossbowBolt.h"

#include <Box2D/Collision/Shapes/b2CircleShape.h>
#include <Box2D/Dynamics/b2Body.h>
#include <Box2D/Dynamics/b2Fixture.h>
#include <Quiver/Entity/Entity.h>
#include <Quiver/Entity/PhysicsComponent/PhysicsComponent.h>
#include <Quiver/Entity/RenderComponent/RenderComponent.h>
#include <Quiver/Misc/Logging.h>
#include <Quiver/World/World.h>

#include "Utils.h"

using namespace qvr;

CrossbowBolt::CrossbowBolt(Entity & entity)
	: CustomComponent(entity)
{
	b2Fixture& fixture = *GetEntity().GetPhysics()->GetBody().GetFixtureList();
	b2Filter filter = fixture.GetFilterData();
	filter.categoryBits = FixtureFilterCategories::CrossbowBolt;
}

void CrossbowBolt::OnBeginContact(Entity & other, b2Fixture & myFixture)
{
	collided = true;
}

auto GetFireAnimation(AnimatorCollection& animators) -> std::experimental::optional<AnimationId>
{
	AnimationSourceInfo fireAnimSource;
	fireAnimSource.filename = "Animations/fire.json";
	
	const AnimationId animation = animators.GetAnimations().GetAnimation(fireAnimSource);

	if (animation != AnimationId::Invalid) {
		return animation;
	}

	if (const auto animData = qvr::AnimationData::FromJsonFile(fireAnimSource.filename))
	{
		return animators.AddAnimation(*animData, fireAnimSource);
	}

	qvr::GetConsoleLogger()->error("Failed to get fire animation");

	return {};
}

auto GetAnimators(CustomComponent& customComponent) -> AnimatorCollection&
{
	return customComponent.GetEntity().GetWorld().GetAnimators();
}

void CrossbowBolt::OnStep(const std::chrono::duration<float> deltaTime)
{
	if (!collided) return;
	
	if (this->effect.appliesEffect == ActiveEffectType::Burning)
	{
		// Mutate into a wee fire

		RenderComponent& renderComp = *GetEntity().GetGraphics();
		
		renderComp.RemoveAnimation();
		if (auto animation = GetFireAnimation(GetAnimators(*this))) {
			renderComp.SetAnimation(*animation);
		}
		renderComp.SetTexture("textures/small_fire.png");
		renderComp.SetSpriteRadius(0.4f);
		renderComp.SetHeight(1.0f);
		renderComp.SetGroundOffset(0.0f);
		renderComp.SetColor(sf::Color::White);
		
		PhysicsComponent& physicsComp = *GetEntity().GetPhysics();

		physicsComp.GetBody().SetType(b2BodyType::b2_staticBody);
		physicsComp.GetBody().GetFixtureList()->SetSensor(true);

		using namespace FixtureFilterCategories;
		SetCategoryBits(*physicsComp.GetBody().GetFixtureList(), Fire);
		SetMaskBits(*physicsComp.GetBody().GetFixtureList(), 0);

		this->GetEntity().AddCustomComponent(nullptr);
	}
	else
	{
		GetEntity().GetWorld().RemoveEntityImmediate(GetEntity());
	}
}