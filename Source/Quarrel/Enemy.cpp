#include <chrono>
#include <iostream>

#include <Box2D/Common/b2Math.h>
#include <Box2D/Collision/Shapes/b2Shape.h>
#include <Box2D/Collision/Shapes/b2CircleShape.h>
#include <Box2D/Dynamics/b2Body.h>
#include <Box2D/Dynamics/b2Fixture.h>
#include <Box2D/Dynamics/b2World.h>
#include <Box2D/Dynamics/b2WorldCallbacks.h>
#include <ImGui/imgui.h>
#include <json.hpp>
#include <optional.hpp>
#include <spdlog/fmt/ostr.h>

#include "Quiver/Animation/AnimationLibrary.h"
#include "Quiver/Animation/AnimationLibraryGui.h"
#include "Quiver/Entity/AudioComponent/AudioComponent.h"
#include "Quiver/Entity/CustomComponent/CustomComponent.h"
#include "Quiver/Entity/PhysicsComponent/PhysicsComponent.h"
#include "Quiver/Entity/RenderComponent/RenderComponent.h"
#include "Quiver/Entity/Entity.h"
#include "Quiver/Misc/ImGuiHelpers.h"
#include "Quiver/Misc/Logging.h"
#include "Quiver/World/World.h"

#include "CrossbowBolt.h"
#include "Utils.h"

using namespace qvr;

namespace
{

using json = nlohmann::json;

using namespace std::chrono;

class EnemyProjectile : public qvr::CustomComponent
{
public:
	EnemyProjectile(qvr::Entity& entity) : CustomComponent(entity) {}

	void OnBeginContact(qvr::Entity& other, b2Fixture& fixture) override
	{
		this->SetRemoveFlag(true);
	}

	std::string GetTypeName() const override { return "EnemyProjectile"; }
};

Entity* MakeProjectile(
	World& world,
	const b2Vec2& position,
	const b2Vec2& aimDir,
	const float speed,
	const b2Vec2& inheritedVelocity,
	const sf::Color& color)
{
	const float radius = 0.1f;

	const b2CircleShape shape = [=]()
	{
		b2CircleShape shape;
		shape.m_radius = radius;
		return shape;
	}();

	Entity* projectile = world.CreateEntity(shape,
		position + aimDir,
		b2Atan2(aimDir.y, aimDir.x) - (b2_pi * 0.5f));

	if (projectile == nullptr) {
		return nullptr;
	}

	// Set up body.
	{
		b2Body& projBody = projectile->GetPhysics()->GetBody();
		projBody.SetType(b2_dynamicBody);
		projBody.SetLinearVelocity((speed * aimDir) + inheritedVelocity);
		projBody.SetBullet(true);

		b2Fixture* fixture = projBody.GetFixtureList();
		b2Filter filterData = fixture->GetFilterData();
		filterData.categoryBits |= FixtureFilterCategories::Projectile;
		fixture->SetFilterData(filterData);
	}

	// Set up RenderComponent.
	{
		projectile->AddGraphics();
		RenderComponent* projRenderComp = projectile->GetGraphics();

		projRenderComp->SetDetached(true);

		projRenderComp->SetHeight(radius * 3);
		projRenderComp->SetSpriteRadius(radius);
		projRenderComp->SetGroundOffset(0.5f);

		projRenderComp->SetColor(color);
	}

	// Set up CustomComponent
	projectile->AddCustomComponent(std::make_unique<EnemyProjectile>(*projectile));

	return projectile;
}

class EntityRef
{
	World * world = nullptr;
public:
	EntityId id = EntityId(0);

	EntityRef() = default;

	EntityRef(World& world, const EntityId id) : world(&world), id(id) {}

	EntityRef(const Entity& entity) : EntityRef(entity.GetWorld(), entity.GetId()) {}

	Entity* Get() {
		if (!world) return nullptr;
		if (id == EntityId(0)) return nullptr;
		if (Entity* entity = world->GetEntity(id)) {
			return entity;
		}
		id = EntityId(0);
		return nullptr;
	}
};

struct ActiveEffect
{
	ActiveEffectType type;
	std::chrono::duration<float> remainingDuration;
};

void AddActiveEffect(
	const ActiveEffectType effectType,
	std::vector<ActiveEffect>& activeEffects)
{
	using namespace std;

	auto AddOrResetDuration = [&](const std::chrono::duration<float> duration) {
		auto it = find_if(
			begin(activeEffects),
			end(activeEffects),
			[effectType](const auto& effect) { return effectType == effect.type; });
		if (it == end(activeEffects)) {
			activeEffects.push_back({ effectType, duration });
		}
		else {
			it->remainingDuration = duration;
		}
	};

	switch (effectType)
	{
	case ActiveEffectType::None: return;
	case ActiveEffectType::Poisoned:
	{
		const auto poisonDuration = 10s;
		AddOrResetDuration(poisonDuration);
		break;
	}
	case ActiveEffectType::Burning:
	{
		const auto burningDuration = 10s;
		AddOrResetDuration(burningDuration);
		break;
	}
	}
}

void ApplyEffect(const ActiveEffect& activeEffect, int& damage)
{
	switch (activeEffect.type)
	{
	case ActiveEffectType::None: assert(false); break;
	case ActiveEffectType::Burning:
		damage += 1;
		break;
	case ActiveEffectType::Poisoned:
		damage += 1;
		break;
	}
}

// Returns true if the effect should be applied.
bool UpdateEffect(ActiveEffect& activeEffect, const std::chrono::duration<float> deltaTime)
{
	const auto oldRemainingTime = activeEffect.remainingDuration;
	activeEffect.remainingDuration -= deltaTime;
	// Only apply effects when we cross over a second.
	const float x = floor(oldRemainingTime.count());
	return activeEffect.remainingDuration.count() < x && oldRemainingTime.count() >= x;
}

class Enemy : public CustomComponent
{
public:
	Enemy(Entity& entity);

	std::string GetTypeName() const override { return "Enemy"; }

	void OnStep(const std::chrono::duration<float> timestep) override;

	void OnBeginContact(Entity& other, b2Fixture& myFixture) override;
	void OnEndContact  (Entity& other, b2Fixture& myFixture) override;

	std::unique_ptr<CustomComponentEditor> CreateEditor() override;

	nlohmann::json ToJson() const override;

	bool FromJson(const nlohmann::json& j) override;

	friend class EnemyEditor;

private:
	bool CanShoot() const;

	void Shoot(const b2Vec2& target);

	void SetAnimation(
		const AnimationId animationId, 
		AnimatorRepeatSetting repeatSetting = AnimatorRepeatSetting::Forever);
	void SetAnimation(std::initializer_list<AnimatorStartSetting> animChain);

	int m_Damage = 0;

	enum class Awakeness {
		None,
		Awakening,
		Awake
	};

	Awakeness m_Awakeness = Awakeness::None;

	bool IsAwake() const {
		return m_Awakeness == Awakeness::Awake;
	}

	World::TimePoint m_LastShootTime = 0s;

	AnimationId m_AwakeAnim = AnimationId::Invalid;
	AnimationId m_RunAnim = AnimationId::Invalid;
	AnimationId m_ShootAnim = AnimationId::Invalid;
	AnimationId m_StandAnim = AnimationId::Invalid;
	AnimationId m_DieAnim = AnimationId::Invalid;

	b2Fixture* m_Sensor;

	EntityRef m_Target;

	std::vector<ActiveEffect> m_ActiveEffects;
};

static b2CircleShape CreateCircleShape(const float radius)
{
	b2CircleShape circle;
	circle.m_radius = radius;
	return circle;
}

Enemy::Enemy(Entity& entity)
	: CustomComponent(entity)
{
	// Create sensor fixture.
	b2FixtureDef fixtureDef;
	b2CircleShape circleShape = CreateCircleShape(5.0f);
	fixtureDef.shape = &circleShape;
	fixtureDef.isSensor = true;
	fixtureDef.filter.categoryBits = FixtureFilterCategories::Sensor;
	// Only collide with Players.
	fixtureDef.filter.maskBits = FixtureFilterCategories::Player;
	m_Sensor = GetEntity().GetPhysics()->GetBody().CreateFixture(&fixtureDef);
}

void Enemy::OnBeginContact(Entity& other, b2Fixture& myFixture)
{
	auto log = qvr::GetConsoleLogger();
	const char* logCtx = "Enemy::OnBeginContact:";

	bool wakeUp = false;

	if (&myFixture == m_Sensor)
	{
		log->debug("{} Player entered sensor.", logCtx);

		m_Target = EntityRef(other);

		wakeUp = true;
	}
	else if (
		other.GetCustomComponent() &&
		other.GetCustomComponent()->GetTypeName() == "CrossbowBolt")
	{
		auto& bolt = *static_cast<CrossbowBolt*>(other.GetCustomComponent());
		
		this->m_Damage += (int)bolt.effect.immediateDamage;

		AddActiveEffect(bolt.effect.appliesEffect, m_ActiveEffects);
		
		// TODO: Get the shooter and set them as the target.

		wakeUp = true;
	}

	if (wakeUp) {
		if (m_Awakeness == Awakeness::None) {
			m_Awakeness = Awakeness::Awakening;
		}
	}
}

void Enemy::OnEndContact(Entity& other, b2Fixture& myFixture)
{
	auto log = qvr::GetConsoleLogger();
	const char* logCtx = "Enemy::OnEndContact:";

	if (&myFixture == m_Sensor)
	{
		log->debug("{} Player left sensor.", logCtx);

		m_Target = EntityRef();
	}
}

void Enemy::OnStep(const std::chrono::duration<float> timestep)
{
	auto log = GetConsoleLogger();

	const auto logCtx = "Enemy::OnStep:";

	for (auto& effect : m_ActiveEffects) {
		if (UpdateEffect(effect, timestep)) {
			ApplyEffect(effect, m_Damage);
		}
	}

	m_ActiveEffects.erase(
		std::remove_if(
			std::begin(m_ActiveEffects),
			std::end(m_ActiveEffects),
			[](const ActiveEffect& effect) { return effect.remainingDuration <= 0s; }),
		std::end(m_ActiveEffects));

	static const int MaxDamage = 10;
	if (m_Damage >= MaxDamage)
	{
		log->debug("{} Dying - received {}/{} damage", logCtx, m_Damage, MaxDamage);
		SetAnimation(m_DieAnim, AnimatorRepeatSetting::Never);
		GetEntity().GetPhysics()->GetBody().DestroyFixture(m_Sensor);
		// Remove the CustomComponent, but not the Entity.
		GetEntity().AddCustomComponent(nullptr);
		// It's super important that this method immediately return after AddCustomComponent!
		return;
	}

	if (m_Awakeness == Awakeness::Awakening) {
		const AnimatorCollection& animSystem = GetEntity().GetWorld().GetAnimators();
		const AnimatorId animator = GetEntity().GetGraphics()->GetAnimatorId();
		if ((!animSystem.Exists(animator)) ||
			(animSystem.GetAnimation(animator) != m_AwakeAnim))
		{
			SetAnimation(
				{
					{ m_AwakeAnim, AnimatorRepeatSetting::Never },
					{ m_StandAnim, AnimatorRepeatSetting::Forever }
				});
		}

		m_Awakeness = Awakeness::Awake;
	}

	if (const Entity* player = m_Target.Get())
	{
		// Check that we have LOS.
		if (const auto visiblePlayerPosition =
			RayCastToFindPlayer(
				*GetEntity().GetWorld().GetPhysicsWorld(),
				GetEntity().GetPhysics()->GetPosition(),
				player->GetPhysics()->GetPosition()))
		{
			if (CanShoot())
			{
				log->debug("{} Firing at position ({}, {})!",
					logCtx,
					visiblePlayerPosition->x,
					visiblePlayerPosition->y);

				Shoot(*visiblePlayerPosition);
			}
		}
	}
}

bool Enemy::CanShoot() const
{
	{
		// Prevent shooting while playing certain anims.
		const AnimatorCollection& animSystem = GetEntity().GetWorld().GetAnimators();
		const AnimatorId animator = GetEntity().GetGraphics()->GetAnimatorId();
		if (animSystem.Exists(animator))
		{
			const AnimationId anim = animSystem.GetAnimation(animator);

			if (anim == m_AwakeAnim ||
				anim == m_ShootAnim)
			{
				return false;
			}
		}
	}

	return GetEntity().GetWorld().GetTime() - m_LastShootTime > 1.0s;
}

void Enemy::Shoot(const b2Vec2& target)
{
	m_LastShootTime = GetEntity().GetWorld().GetTime();
	
	SetAnimation(
		{
			{ m_ShootAnim, AnimatorRepeatSetting::Never },
			{ m_StandAnim, AnimatorRepeatSetting::Forever }
		});
	
	if (!GetEntity().GetAudio())
	{
		GetEntity().AddAudio();
	}

	GetEntity().GetAudio()->SetSound("audio/crossbow_shoot.wav");

	const auto position = GetEntity().GetPhysics()->GetPosition();
	auto direction = target - position;
	direction.Normalize();

	MakeProjectile(
		GetEntity().GetWorld(),
		position,
		direction,
		20.0f,
		b2Vec2_zero,
		sf::Color::Red);
}

void Enemy::SetAnimation(std::initializer_list<AnimatorStartSetting> animChain)
{
	auto log = GetConsoleLogger();
	const char* logCtx = "Enemy::SetAnimation:";

	assert(animChain.size() > 0);

	const AnimatorStartSetting first = *animChain.begin();

	Enemy::SetAnimation(first.m_AnimationId, first.m_RepeatSetting);

	const AnimatorId animator = GetEntity().GetGraphics()->GetAnimatorId();

	if (animChain.size() > 1) {
		log->debug("{} Queuing {} Animations...", logCtx, animChain.size() - 1);
	}

	for (auto it = animChain.begin() + 1; it != animChain.end(); ++it)
	{
		if (!GetEntity().GetWorld().GetAnimators().QueueAnimation(animator, *it)) {
			log->warn("{} Couldn't queue Animation {}", logCtx, it->m_AnimationId);
			continue;
		}
		log->debug("{} Queued Animation {}", logCtx, it->m_AnimationId);
	}
}

void Enemy::SetAnimation(const AnimationId animationId, const AnimatorRepeatSetting repeatSetting)
{
	auto log = GetConsoleLogger();
	const char* logCtx = "Enemy::SetAnimation:";

	if (animationId != AnimationId::Invalid)
	{
		if (!GetEntity().GetGraphics()->SetAnimation(animationId, repeatSetting))
		{
			log->warn("{} Couldn't set the RenderComponent's Animation to {}", logCtx, animationId);
		}
	}
}

optional<AnimationId> PickAnimationGui(
	const char* title, 
	const AnimationId currentAnim, 
	const AnimatorCollection& animators)
{
	if (ImGui::CollapsingHeader(title))
	{
		ImGui::AutoIndent autoIndent;

		if (currentAnim != AnimationId::Invalid)
		{
			if (auto animSourceInfo = animators.GetAnimations().GetSourceInfo(currentAnim))
			{
				ImGui::Text("Animation ID: %d", currentAnim.GetValue());
				ImGui::Text("Animation Data Source:");
				ImGui::Text("  Name: %s", animSourceInfo->name.c_str());
				ImGui::Text("  File: %s", animSourceInfo->filename.c_str());
			}
			
			if (ImGui::Button(fmt::format("No {}", title).c_str()))
			{
				return AnimationId::Invalid;
			}
		}
		else
		{
			int selection = -1;
			return PickAnimation(animators.GetAnimations(), selection);
		}
	}

	return {};
}

class EnemyEditor : public CustomComponentEditorType<Enemy>
{
public:
	EnemyEditor(Enemy& player) : CustomComponentEditorType(player) {}

	void GuiControls() override {
		AnimatorCollection& animSystem = Target().GetEntity().GetWorld().GetAnimators();

		if (const auto animationId = PickAnimationGui("Run Animation", Target().m_RunAnim, animSystem))
		{
			Target().m_RunAnim = animationId.value();
		}

		if (const auto animationId = PickAnimationGui("Idle Animation", Target().m_StandAnim, animSystem))
		{
			Target().m_StandAnim = animationId.value();
		}

		if (const auto animationId = PickAnimationGui("Shoot Animation", Target().m_ShootAnim, animSystem))
		{
			Target().m_ShootAnim = animationId.value();
		}

		if (const auto animationId = PickAnimationGui("Die Animation", Target().m_DieAnim, animSystem))
		{
			Target().m_DieAnim = animationId.value();
		}

		if (const auto animationId = PickAnimationGui("Awake Animation", Target().m_AwakeAnim, animSystem))
		{
			Target().m_AwakeAnim = animationId.value();
		}
	}
};

std::unique_ptr<CustomComponentEditor> Enemy::CreateEditor() {
	return std::make_unique<EnemyEditor>(*this);
}

json AnimationToJson(const AnimatorCollection& animators, const AnimationId animationId)
{
	if (animationId == AnimationId::Invalid) return {};
	
	auto log = GetConsoleLogger();

	const std::string logCtx = fmt::format("AnimationToJson({}):", animationId);

	if (!animators.GetAnimations().Contains(animationId))
	{
		log->error("{} Animation does not exist", logCtx);
		return {};
	}

	const auto animSource = animators.GetAnimations().GetSourceInfo(animationId);
	
	if (!animSource)
	{
		log->error("{} Could not retrieve any Source Info for this Animation", logCtx);
		return {};
	}

	json j = animSource.value();

	return j;
}

AnimationId AnimationFromJson(const AnimatorCollection& animators, const nlohmann::json& j)
{
	AnimationSourceInfo animSource = j;

	return animators.GetAnimations().GetAnimation(animSource);
}

json Enemy::ToJson() const
{
	const AnimatorCollection& animSystem = GetEntity().GetWorld().GetAnimators();

	json j;

	j["RunAnim"] = AnimationToJson(animSystem, m_RunAnim);
	j["ShootAnim"] = AnimationToJson(animSystem, m_ShootAnim);
	j["IdleAnim"] = AnimationToJson(animSystem, m_StandAnim);
	j["DieAnim"] = AnimationToJson(animSystem, m_DieAnim);
	j["AwakeAnim"] = AnimationToJson(animSystem, m_AwakeAnim);

	return j;
}

bool Enemy::FromJson(const nlohmann::json& j)
{
	AnimatorCollection& animSystem = GetEntity().GetWorld().GetAnimators();

	m_RunAnim = AnimationFromJson(animSystem, j.value<json>("RunAnim", {}));
	m_ShootAnim = AnimationFromJson(animSystem, j.value<json>("ShootAnim", {}));
	m_StandAnim = AnimationFromJson(animSystem, j.value<json>("IdleAnim", {}));
	m_DieAnim = AnimationFromJson(animSystem, j.value<json>("DieAnim", {}));
	m_AwakeAnim = AnimationFromJson(animSystem, j.value<json>("AwakeAnim", {}));

	return true;
}

}

std::unique_ptr<CustomComponent> CreateEnemy(Entity& entity)
{
	return std::make_unique<Enemy>(entity);
}
