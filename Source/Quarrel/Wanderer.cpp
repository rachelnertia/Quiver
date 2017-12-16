#include "Wanderer.h"

#include <Box2D/Dynamics/b2Body.h>
#include <Box2D/Dynamics/b2Fixture.h>
#include <Box2D/Dynamics/b2World.h>
#include <ImGui/imgui.h>
#include <spdlog/spdlog.h>

#include "Quiver/Animation/Animators.h"
#include "Quiver/Entity/Entity.h"
#include "Quiver/Entity/PhysicsComponent/PhysicsComponent.h"
#include "Quiver/Entity/RenderComponent/RenderComponent.h"
#include "Quiver/World/World.h"

using namespace qvr;

Wanderer::Wanderer(Entity& entity) 
	: CustomComponent(entity) 
{
	mWalkingState.startPos = GetEntity().GetPhysics()->GetPosition();
	mWalkingState.direction = b2Vec2(-1.0f, 0.0f);
}

Wanderer::~Wanderer() {}

bool IsPathClear(const b2World& world, b2Vec2 start, b2Vec2 direction, float distance) {
	class RayCastCallback : public b2RayCastCallback {
	public:
		inline float32 ReportFixture(b2Fixture* fixture, const b2Vec2& point,
			const b2Vec2& normal, float32 fraction)
			override
		{
			// I'm not sure that Box2D will create contacts between these fixture,
			// but just to be safe, hitSomething takes into acount the RenderComponent
			// FlatSprite filter data.
			// Ignore members of the detached-RenderComponent category (0xF000).
			hitSomething = (fixture->GetFilterData().categoryBits & 0xF000) == 0;
			return 0;
		}

		bool hitSomething = false;
	};

	RayCastCallback cb;

	world.RayCast(&cb, start, start + direction);

	return !cb.hitSomething;
}

void Wanderer::OnStep(float timestep)
{
	if (playerContacts > 0) {
		SetRemoveFlag(true);
		return;
	}

	// Walk in direction until a certain distance travelled is exceeded or
	// an object gets in the way (do a raycast to find out).
	if (mState == State::Walking) {
		const b2World& world = *GetEntity().GetWorld().GetPhysicsWorld();
		const b2Vec2 currentPos = GetEntity().GetPhysics()->GetPosition();

		if (IsPathClear(world, currentPos, mWalkingState.direction, 1.0f)) {
			b2Body& body = GetEntity().GetPhysics()->GetBody();
			float walkForce = 2.0f;
			body.ApplyForce(walkForce * mWalkingState.direction,
				b2Vec2(0.0f, 0.0f),
				true);
		} else {
			// Just flip direction.
			mWalkingState.direction *= -1.0f;

			GetEntity().GetGraphics()->SetAnimatorRotation(
				b2Atan2(mWalkingState.direction.y, mWalkingState.direction.x) + b2_pi);
		}
	}
	// Rotate direction until there's a clear path or we've turned 180 degrees.
	else if (mState == State::Turning) {

	}
}

void Wanderer::OnBeginContact(Entity& other)
{
	auto log = spdlog::get("console");
	assert(log);

	if (other.GetCustomComponent()) {
		log->debug("Wanderer beginning contact with {}...", other.GetCustomComponent()->GetTypeName());

		if (other.GetCustomComponent()->GetTypeName() == "Player") {
			SetRemoveFlag(true);
		}
	}
}

void Wanderer::OnEndContact(Entity& other)
{
	auto log = spdlog::get("console");
	assert(log);

	if (other.GetCustomComponent()) {
		log->debug("Wanderer finishing contact with {}...", other.GetCustomComponent()->GetTypeName());
	}
}

void Wanderer::GUIControls()
{
	
}