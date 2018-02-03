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
#include <spdlog/spdlog.h>

#include "Quiver/Animation/AnimationLibrary.h"
#include "Quiver/Animation/AnimationLibraryGui.h"
#include "Quiver/Entity/AudioComponent/AudioComponent.h"
#include "Quiver/Entity/CustomComponent/CustomComponent.h"
#include "Quiver/Entity/PhysicsComponent/PhysicsComponent.h"
#include "Quiver/Entity/RenderComponent/RenderComponent.h"
#include "Quiver/Entity/Entity.h"
#include "Quiver/Misc/ImGuiHelpers.h"
#include "Quiver/World/World.h"

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

class Enemy : public CustomComponent
{
public:
	Enemy(Entity& entity);

	std::string GetTypeName() const override { return "Enemy"; }

	void OnStep(const std::chrono::duration<float> timestep) override;

	void OnBeginContact(Entity& other, b2Fixture& myFixture) override
	{
		if (other.GetCustomComponent() && 
			(other.GetCustomComponent()->GetTypeName() == "CrossbowBolt"))
		{
			m_Damage += 10;
		}
	}

	std::unique_ptr<CustomComponentEditor> CreateEditor() override;

	nlohmann::json ToJson() const override;

	bool FromJson(const nlohmann::json& j) override;

	friend class EnemyEditor;

private:
	struct PlayerSighting
	{
		b2Vec2 position;
		World::TimePoint worldTime;
	};

	bool CanShoot() const;

	void Shoot();

	void SetAnimation(const AnimationId animationId, AnimatorRepeatSetting repeatSetting = AnimatorRepeatSetting::Forever);
	void SetAnimation(std::initializer_list<AnimatorStartSetting> animChain);

	b2Vec2 GetDirection() const;

	std::experimental::optional<PlayerSighting> m_LastPlayerSighting;

	float m_CurrentAngle = 0.0f;
	int m_Damage = 0;
	bool m_Awake = false;

	World::TimePoint m_LastShootTime = 0s;

	AnimationId m_AwakeAnim = AnimationId::Invalid;
	AnimationId m_RunAnim = AnimationId::Invalid;
	AnimationId m_ShootAnim = AnimationId::Invalid;
	AnimationId m_StandAnim = AnimationId::Invalid;
	AnimationId m_DieAnim = AnimationId::Invalid;
};

Enemy::Enemy(Entity& entity)
	: CustomComponent(entity)
{}

void Enemy::OnStep(const std::chrono::duration<float> timestep)
{
	auto log = spdlog::get("console");
	assert(log);

	const auto logCtx = "Enemy::OnStep:";

	static const int MaxDamage = 10;
	if (m_Damage >= MaxDamage)
	{
		log->debug("{} Dying - received {}/{} damage", logCtx, m_Damage, MaxDamage);
		SetAnimation(m_DieAnim, AnimatorRepeatSetting::Never);
		// Remove the CustomComponent, but not the Entity.
		GetEntity().AddCustomComponent(nullptr);
		// It's super important that this method immediately return after AddCustomComponent!
		return;
	}

	const b2Body&  body = GetEntity().GetPhysics()->GetBody();
	const b2World& physicsWorld = *body.GetWorld();

	const b2Vec2 currentPos = body.GetPosition();

	const auto currentTime = GetEntity().GetWorld().GetTime();

	if (m_LastPlayerSighting)
	{
		const PlayerSighting& lastPlayerSighting = m_LastPlayerSighting.value();

		{
			const b2Vec2 dir = lastPlayerSighting.position - currentPos;
			m_CurrentAngle = b2Atan2(dir.y, dir.x);
		}

		// Find the player again because it's unsafe to hold a pointer to the Player Entity/CustomComponent.
		{
			const float halfWidth = 0.5f;
			const b2Vec2 halfDimensions = b2Vec2{ halfWidth, halfWidth };
			const b2AABB aabb
			{
				lastPlayerSighting.position - halfDimensions,
				lastPlayerSighting.position + halfDimensions
			};

			if (const auto playerPosition = QueryAABBToFindPlayer(physicsWorld, aabb))
			{
				// Check that we can see the Player directly.
				if (RayCastToFindPlayer(physicsWorld, currentPos, playerPosition.value()))
				{
					// Update the LastPlayerSighting struct.
					m_LastPlayerSighting =
						PlayerSighting
						{
							playerPosition.value(),
							currentTime
						};

					// Shoot at player.
					if (CanShoot())
					{
						log->debug("{} Firing at position ({}, {})!",
							logCtx,
							lastPlayerSighting.position.x,
							lastPlayerSighting.position.y);

						Shoot();
					}
				}
			}
		}

		// If it's been a while since we saw the Player, forget it.
		if (GetEntity().GetWorld().GetTime() - lastPlayerSighting.worldTime > 5s)
		{
			log->debug("{} Last player sighting (position: ({}, {}), time: {}s) timed out. Forgetting...",
				logCtx,
				lastPlayerSighting.position.x,
				lastPlayerSighting.position.y,
				lastPlayerSighting.worldTime.count());

			// Forget it.
			m_LastPlayerSighting = {};
		}
	} 
	else
	{
		if (const auto playerPosition = RayCastToFindPlayer(physicsWorld, currentPos, m_CurrentAngle, 5.0f))
		{
			m_LastPlayerSighting =
				PlayerSighting
				{
					playerPosition.value(),
					currentTime
				};

			log->debug("{} New player sighting at (position: ({}, {}), time: {}s).",
				logCtx,
				m_LastPlayerSighting.value().position.x,
				m_LastPlayerSighting.value().position.y,
				m_LastPlayerSighting.value().worldTime.count());

			if (!m_Awake)
			{
				m_Awake = true;
				
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
			}
			else
			{
				SetAnimation(m_StandAnim);
			}
		} 
		else
		{
			m_CurrentAngle += b2_pi * timestep.count();
		}
	}
}

bool Enemy::CanShoot() const
{
	{
		// Prevent shooting while playing the awakening anim.
		const AnimatorCollection& animSystem = GetEntity().GetWorld().GetAnimators();
		const AnimatorId animator = GetEntity().GetGraphics()->GetAnimatorId();
		if (animSystem.Exists(animator) &&
			animSystem.GetAnimation(animator) == m_AwakeAnim)
		{
			return false;
		}
	}

	return GetEntity().GetWorld().GetTime() - m_LastShootTime > 1.0s;
}

void Enemy::Shoot()
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
	const auto direction = GetDirection();

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
	auto log = spdlog::get("console");
	assert(log);
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
	auto log = spdlog::get("console");
	assert(log);
	const char* logCtx = "Enemy::SetAnimation:";

	if (animationId != AnimationId::Invalid)
	{
		if (!GetEntity().GetGraphics()->SetAnimation(animationId, repeatSetting))
		{
			log->warn("{} Couldn't set the RenderComponent's Animation to {}", logCtx, animationId);
		}
	}
}

b2Vec2 Enemy::GetDirection() const
{
	return b2Mul(b2Rot(m_CurrentAngle), b2Vec2(1.0f, 0.0f));
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
	
	auto log = spdlog::get("console");
	assert(log);

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
