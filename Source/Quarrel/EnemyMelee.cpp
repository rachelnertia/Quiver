#include "EnemyMelee.h"

#include <Box2D/Collision/Shapes/b2CircleShape.h>
#include <Box2D/Collision/Shapes/b2PolygonShape.h>
#include <Box2D/Dynamics/b2Body.h>
#include <Box2D/Dynamics/b2Fixture.h>

#include <Quiver/Entity/Entity.h>
#include <Quiver/Entity/CustomComponent/CustomComponent.h>
#include <Quiver/Entity/PhysicsComponent/PhysicsComponent.h>
#include <Quiver/Entity/RenderComponent/RenderComponent.h>
#include <Quiver/World/World.h>

#include "Damage.h"
#include "Effects.h"
#include "FirePropagation.h"
#include "GuiUtils.h"
#include "Gravity.h"
#include "Utils.h"

using namespace std::chrono_literals;

using seconds = std::chrono::duration<float>;

class EnemyMelee : public qvr::CustomComponent
{
	b2Fixture* sensorFixture = nullptr;
	b2Fixture* attackSwipeFixture = nullptr;
	
	EntityRef target;

	float upVelocity = 0.0f;

	DamageCount damageCounter = DamageCount(30);
	ActiveEffectSet activeEffects;
	FiresInContact firesInContact;

	qvr::AnimationId attackAnimation;
	qvr::AnimationId dieAnimation;

	std::function<void()> pendingAction;

	bool IsAttacking();

	void DestroyAttackSwipeFixture() {
		if (attackSwipeFixture) {
			GetEntity().GetPhysics()->GetBody().DestroyFixture(attackSwipeFixture);
			attackSwipeFixture = nullptr;
		}
	}

public:
	EnemyMelee(qvr::Entity& entity);

	std::string GetTypeName() const override {
		return "EnemyMelee";
	}

	void OnStep(const seconds deltaTime) override;

	void OnBeginContact(
		qvr::Entity& other,
		b2Fixture& myFixture,
		b2Fixture& otherFixture) override;

	void OnEndContact(
		qvr::Entity& other,
		b2Fixture& myFixture,
		b2Fixture& otherFixture) override;

	std::unique_ptr<qvr::CustomComponentEditor> CreateEditor() override;

	nlohmann::json ToJson() const override;

	bool FromJson(const nlohmann::json& j) override;

	friend class EnemyMeleeEditor;
};

class EnemyMeleeEditor : public qvr::CustomComponentEditorType<EnemyMelee>
{
public:
	EnemyMeleeEditor(EnemyMelee& target) : CustomComponentEditorType(target) {}

	void GuiControls() override {
		qvr::AnimatorCollection& animSystem = Target().GetEntity().GetWorld().GetAnimators();

		if (const auto animationId = PickAnimationGui("Attack Animation", Target().attackAnimation, animSystem))
		{
			Target().attackAnimation = animationId.value();
		}

		if (const auto animationId = PickAnimationGui("Die Animation", Target().dieAnimation, animSystem))
		{
			Target().dieAnimation = animationId.value();
		}
	}
};

std::unique_ptr<qvr::CustomComponentEditor> EnemyMelee::CreateEditor() {
	return std::make_unique<EnemyMeleeEditor>(*this);
}

std::unique_ptr<qvr::CustomComponent> CreateEnemyMelee(qvr::Entity& entity) {
	return std::make_unique<EnemyMelee>(entity);
}

using Radius = fluent::NamedType<float, struct RadiusTag>;

b2Fixture* CreateSensorFixture(
	b2Body& body, 
	const Radius radius, 
	const uint16 collidesWithCategories) 
{
	b2FixtureDef fixtureDef;
	b2CircleShape circleShape = CreateCircleShape(radius.get());
	fixtureDef.shape = &circleShape;
	fixtureDef.isSensor = true;
	fixtureDef.filter.categoryBits = FixtureFilterCategories::Sensor;
	fixtureDef.filter.maskBits = collidesWithCategories;
	return body.CreateFixture(&fixtureDef);
}

b2Fixture* CreateAttackSwipeFixture(b2Body& body, const Radius radius) {
	b2FixtureDef fixtureDef;
	b2CircleShape shape = CreateCircleShape(radius.get());
	fixtureDef.shape = &shape;
	fixtureDef.isSensor = true;
	fixtureDef.filter.categoryBits = FixtureFilterCategories::EnemyAttack;
	fixtureDef.filter.maskBits = FixtureFilterCategories::Player;
	return body.CreateFixture(&fixtureDef);
}

EnemyMelee::EnemyMelee(qvr::Entity& entity)
	: CustomComponent(entity)
{
	if (GetEntity().GetPhysics()) {
		SetCategoryBits(
			*GetEntity().GetPhysics()->GetBody().GetFixtureList(),
			FixtureFilterCategories::Enemy);

		// Create sensor fixture that only collides with Players.
		sensorFixture = 
			CreateSensorFixture(
				GetEntity().GetPhysics()->GetBody(),
				Radius(5.0f),
				FixtureFilterCategories::Player);
	}
}

void EnemyMelee::OnBeginContact(
	qvr::Entity& other,
	b2Fixture& myFixture,
	b2Fixture& otherFixture)
{
	if (&myFixture == sensorFixture) {
		if (target.Get() == nullptr) {
			target = EntityRef(other);
		}

		return;
	}

	if (&myFixture == attackSwipeFixture) {
		pendingAction = [this]() -> void {
			DestroyAttackSwipeFixture();
		};

		return;
	}
	
	if (IsCrossbowBolt(otherFixture))
	{
		HandleContactWithCrossbowBolt(other, damageCounter);
		HandleContactWithCrossbowBolt(other, activeEffects);
	}

	::OnBeginContact(firesInContact, otherFixture);
}

void EnemyMelee::OnEndContact(
	qvr::Entity& other,
	b2Fixture& myFixture,
	b2Fixture& otherFixture)
{
	if (&myFixture == sensorFixture) {
		if (target.Get() == &other) {
			target = {};
		}
	}
	else {
		::OnEndContact(firesInContact, otherFixture);
	}
}

void MoveTowardsPosition(b2Body& body, const b2Vec2& targetPosition, const float speed) {
	b2Vec2 velocity = targetPosition - body.GetPosition();
	velocity.Normalize();
	velocity *= speed;

	body.SetLinearVelocity(velocity);
}

void EnemyMelee::OnStep(const seconds deltaTime) 
{
	if (pendingAction) {
		pendingAction();
		pendingAction = nullptr;
	}

	ApplyFires(firesInContact, activeEffects);
	
	for (auto& effect : activeEffects.container) {
		if (UpdateEffect(effect, deltaTime)) {
			ApplyEffect(effect, damageCounter);
		}
		ApplyEffect(effect, *GetEntity().GetGraphics());
	}

	RemoveExpiredEffects(activeEffects);

	if (HasExceededLimit(damageCounter)) {
		DestroyAttackSwipeFixture();
		
		GetEntity().GetGraphics()->SetAnimation(
			dieAnimation,
			qvr::AnimatorRepeatSetting::Never);
		
		GetEntity().AddCustomComponent(nullptr);
		return;
	}

	if (!IsAttacking()) {
		DestroyAttackSwipeFixture();

		qvr::Entity* targetEntity = target.Get();

		if (targetEntity) {
			const float distance =
				(GetEntity().GetPhysics()->GetPosition() - targetEntity->GetPhysics()->GetPosition()).Length();

			const float attackDistance = 1.0f;

			if (distance > attackDistance) {
				const float speed = 1.0f;

				MoveTowardsPosition(
					GetEntity().GetPhysics()->GetBody(),
					targetEntity->GetPhysics()->GetPosition(),
					speed);

				if (GetEntity().GetGraphics()->GetGroundOffset() <= 0.0f) {
					const float jumpVelocity = 2.0f;
					upVelocity = jumpVelocity;
				}
			}
			else {
				qvr::RenderComponent& graphics = *GetEntity().GetGraphics();

				graphics.SetAnimation(attackAnimation, qvr::AnimatorRepeatSetting::Never);

				if (attackSwipeFixture == nullptr) {
					attackSwipeFixture =
						CreateAttackSwipeFixture(
							GetEntity().GetPhysics()->GetBody(),
							Radius(1.0f));
				}
			}
		}
	}

	upVelocity = ApplyGravity(upVelocity, deltaTime);
	
	UpdateGroundOffset(*GetEntity().GetGraphics(), GetPositionDelta(upVelocity, deltaTime));
}

bool EnemyMelee::IsAttacking() {
	return GetCurrentAnimation(GetEntity()) == attackAnimation;
}

using json = nlohmann::json;

json EnemyMelee::ToJson() const {
	const auto& animSystem = GetEntity().GetWorld().GetAnimators();

	return json{
		{"AttackAnim", AnimationToJson(animSystem, attackAnimation)},
		{"DieAnim", AnimationToJson(animSystem, dieAnimation)}
	};
}

bool EnemyMelee::FromJson(const nlohmann::json& j) {
	auto& animSystem = GetEntity().GetWorld().GetAnimators();

	attackAnimation = AnimationFromJson(animSystem, j.value<json>("AttackAnim", {}));
	dieAnimation = AnimationFromJson(animSystem, j.value<json>("DieAnim", {}));

	return true;
}