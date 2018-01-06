#pragma once

#include "Quiver/Entity/Component.h"

#include <Box2D/Common/b2Math.h>
#include <SFML/Graphics/Color.hpp>
#include <json.hpp>

#include "Quiver/Animation/Animators.h"
#include "Quiver/Graphics/FixtureRenderData.h"
#include "Quiver/Physics/PhysicsUtils.h"

class b2Fixture;
class b2Body;

namespace qvr {

class RenderComponent : public Component {
public:
	explicit RenderComponent(Entity& entity);
	~RenderComponent();

	RenderComponent(const RenderComponent&) = delete;
	RenderComponent(const RenderComponent&&) = delete;

	RenderComponent& operator=(const RenderComponent&) = delete;
	RenderComponent& operator=(const RenderComponent&&) = delete;

	bool ToJson(nlohmann::json& j) const;
	bool FromJson(const nlohmann::json& j);

	void UpdateDetachedBodyRotation(const float cameraAngle);
	void UpdateDetachedBodyPosition();

	float GetHeight()                 const { return mFixtureRenderData->GetHeight(); }
	float GetGroundOffset()           const { return mFixtureRenderData->GetGroundOffset(); }
	float GetSpriteRadius()           const { return mFixtureRenderData->GetSpriteRadius(); }
	float GetObjectAngle()            const { return mFixtureRenderData->GetObjectAngle(); }
	const b2Vec2& GetSpritePosition() const { return mFixtureRenderData->GetSpritePosition(); }
	const sf::Color GetColor()        const { return mFixtureRenderData->GetColor(); }

	void SetHeight      (const float height)       { mFixtureRenderData->mHeight = height; }
	void SetGroundOffset(const float groundOffset) { mFixtureRenderData->mGroundOffset = groundOffset; }
	void SetObjectAngle (const float radians)      { mFixtureRenderData->mObjectAngle = radians; }
	void SetColor       (const sf::Color& color)   { mFixtureRenderData->mBlendColor = color; }
	void SetSpriteRadius(const float spriteRadius);

	const sf::Texture* GetTexture()         const { return mFixtureRenderData->GetTexture(); }
	const char*        GetTextureFilename() const { return mTextureFilename.c_str(); }
	bool SetTexture(const std::string& filename);
	void RemoveTexture();

	const ViewBuffer& GetViews() const { return mFixtureRenderData->GetViews(); }

	void SetTextureRect(const Animation::Rect& rect);

	AnimatorId GetAnimatorId() { return mAnimatorId; }

	bool SetAnimation(const AnimationId animationId);
	bool SetAnimation(const AnimationId animationId, AnimatorRepeatSetting repeatSetting);

	void RemoveAnimation();
	
	// Returns true if the RenderComponent has its own (non-collidable) physical body.
	bool IsDetached() const { return mDetachedBody != nullptr; }

	void SetDetached(const bool detached);

private:
	b2Body* GetDetachedBody() { return mDetachedBody.get(); }
	b2Body* GetBody();
	b2Fixture* GetFixture();
	
	AnimatorId mAnimatorId = AnimatorId::Invalid;

	std::string mTextureFilename;

	std::unique_ptr<qvr::FixtureRenderData> mFixtureRenderData;

	// Set when the RenderComponent has a different b2Body from the PhysicsComponent.
	Physics::b2BodyUniquePtr mDetachedBody;
};

}