#include "RenderComponent.h"

#include <Box2D/Collision/Shapes/b2EdgeShape.h>
#include <Box2D/Dynamics/b2Body.h>
#include <Box2D/Dynamics/b2Fixture.h>
#include <Box2D/Dynamics/b2World.h>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Shader.hpp>
#include <SFML/Graphics/Texture.hpp>

#include "Quiver/Entity/Entity.h"
#include "Quiver/Entity/PhysicsComponent/PhysicsComponent.h"
#include "Quiver/Graphics/Camera3D.h"
#include "Quiver/Graphics/ColourUtils.h"
#include "Quiver/Graphics/Light.h"
#include "Quiver/Graphics/TextureLibrary.h"
#include "Quiver/Misc/Logging.h"
#include "Quiver/World/World.h"

namespace
{

using namespace qvr;

AnimationSystem& GetAnimationSystem(const RenderComponent& renderComponent) {
	return renderComponent.GetEntity().GetWorld().GetAnimationSystem();
}

TextureLibrary& GetTextureLibrary(const RenderComponent& renderComponent) {
	return renderComponent.GetEntity().GetWorld().GetTextureLibrary();
}

b2World* GetPhysicsWorld(RenderComponent& rc) {
	return const_cast<b2World*>(rc.GetEntity().GetWorld().GetPhysicsWorld());
}

Animation::Rect SfVecToRect(const sf::Vector2u& v) {
	Animation::Rect rect;
	rect.top = 0;
	rect.left = 0;
	rect.bottom = v.y;
	rect.right = v.x;
	return rect;
}

}

namespace qvr {

b2Fixture* RenderComponent::GetFixture() {
	return GetBody()->GetFixtureList();
}

b2Body* RenderComponent::GetBody() {
	return GetDetachedBody() != nullptr ?
		GetDetachedBody() :
		&GetEntity().GetPhysics()->GetBody();
}

RenderComponent::RenderComponent(Entity& entity)
	: Component(entity)
	, mFixtureRenderData(std::make_unique<qvr::FixtureRenderData>())
{
	GetFixture()->SetUserData(mFixtureRenderData.get());
}

RenderComponent::~RenderComponent()
{
	auto log = GetConsoleLogger();

	if (mAnimatorId != AnimatorId::Invalid) {
		if (GetEntity().GetWorld().GetAnimationSystem().RemoveAnimator(mAnimatorId)) {
			log->debug("RenderComponent::Destroy: Removed Animator {}", mAnimatorId);
		}
		else {
			log->error("RenderComponent::Destroy: RemoveAnimator({}) FAILED!", mAnimatorId);
		}
		mAnimatorId = AnimatorId::Invalid;
	}

	GetFixture()->SetUserData(nullptr);

	if (mDetachedBody) {
		GetEntity().GetWorld().UnregisterDetachedRenderComponent(*this);
		GetEntity().GetWorld().UnregisterAnimatorWithAltViews(*this);
	}
}

bool RenderComponent::ToJson(nlohmann::json & j) const
{
	j["Detached"] = IsDetached();
	j["Height"] = GetHeight();
	j["GroundOffset"] = GetGroundOffset();
	j["Colour"] = ColourUtils::ToJson(GetColor());
	j["SpriteRadius"] = GetSpriteRadius();

	if (GetTexture()) {
		j["Texture"] = mTextureFilename;
	}

	{
		const AnimationSystem& animSystem = GetAnimationSystem(*this);

		if ((mAnimatorId != AnimatorId::Invalid) && animSystem.AnimatorExists(mAnimatorId))
		{
			const AnimationId animId = animSystem.GetAnimatorAnimation(mAnimatorId);

			if (const auto source = animSystem.GetAnimations().GetSourceInfo(animId)) 
			{
				j["Animation"]["File"] = source->filename;
				if (!source->name.empty()) {
					j["Animation"]["Name"] = source->name;
				}
			}

			if (animSystem.GetAnimatorFrame(mAnimatorId) > 0) {
				j["Animation"]["CurrentFrame"] = animSystem.GetAnimatorFrame(mAnimatorId);
			}
		}
		else if (GetTexture())
		{
			GetTextureRect().ToJson(j["TextureRect"]);
		}
	}

	return true;
}

bool RenderComponent::FromJson(const nlohmann::json & j)
{
	auto log = GetConsoleLogger();

	if (!VerifyJson(j)) {
		return false;
	}

	SetHeight(j.value<float>("Height", 1.0f));
	SetGroundOffset(j.value<float>("GroundOffset", 0.0f));

	if (j.count("Colour") &&
		!ColourUtils::DeserializeSFColorFromJson(
			mFixtureRenderData->mBlendColor,
			j["Colour"]))
	{
		return false;
	}
	
	{
		const auto renderType = j.value<std::string>("RenderType", {});
		const auto detached = j.value<bool>("Detached", false);

		if (renderType == "Sprite" || detached) {
			SetDetached(true);
		}
	}

	SetSpriteRadius(j.value<float>("SpriteRadius", 0.5f));

	auto FieldExistsInJson = [](std::string fieldName, const nlohmann::json& j)
	{
		return j.find(fieldName) != j.end();
	};

	if (FieldExistsInJson("Texture", j)) {
		if (j["Texture"].is_string()) {
			std::string filename = j["Texture"].get<std::string>();
			if (filename.length() > 0) {
				mFixtureRenderData->mTexture = GetEntity().GetWorld().GetTextureLibrary().LoadTexture(filename);
				if (GetTexture()) {
					mTextureFilename = filename;
					mFixtureRenderData->mTextureRect.rect = SfVecToRect(GetTexture()->getSize());
				}
				else {
					mTextureFilename.clear();
				}
			}
		}
		else {
			log->error("Texture field must be a filename (string).");
		}

		if (FieldExistsInJson("TextureRect", j))
		{
			mFixtureRenderData->mTextureRect.rect.FromJson(j["TextureRect"]);
		}
	}

	if (FieldExistsInJson("Animation", j)) {
		const AnimationSourceInfo animSource = j["Animation"];
		
		if (!animSource.filename.empty()) {
			AnimationSystem& animSystem = GetEntity().GetWorld().GetAnimationSystem();

			AnimationId animId = animSystem.GetAnimations().GetAnimation(animSource);

			if (animId != AnimationId::Invalid) {
				if (SetAnimation(animId)) {
					if (FieldExistsInJson("CurrentFrame", j["Animation"])) {
						const unsigned currentFrame = j["Animation"]["CurrentFrame"].get<unsigned>();

						animSystem.SetAnimatorFrame(mAnimatorId, currentFrame);
					}
				}
			}
		}
	}

	// All done!

	return true;
}

bool RenderComponent::VerifyJson(const nlohmann::json & j)
{
	auto log = GetConsoleLogger();

	if (j.empty()) {
		log->error("JSON object is empty!");
		return false;
	}

	if (j.count("Height")       && !j["Height"].is_number())       return false;
	if (j.count("GroundOffset") && !j["GroundOffset"].is_number()) return false;

	if (j.count("Colour") && !ColourUtils::VerifyColourJson(j["Colour"])) return false;
	
	if (j.find("Texture") != j.end()) {
		if (!j["Texture"].is_string()) {
			log->error("Texture field must be a filename (string).");
		}
	}

	if (j.find("SpriteRadius") != j.end()) {
		if (!j["SpriteRadius"].is_number()) {
			log->error("SpriteRadius field must be a number.");
		}
	}

	return true;
}

void RenderComponent::UpdateDetachedBodyRotation(const float cameraAngle)
{
	assert(IsDetached());

	mDetachedBody->SetTransform(mDetachedBody->GetPosition(), cameraAngle);
}

void RenderComponent::UpdateDetachedBodyPosition()
{
	assert(IsDetached());

	const b2Vec2 position = GetEntity().GetPhysics()->GetPosition();

	mDetachedBody->SetTransform(position, mDetachedBody->GetAngle());

	mFixtureRenderData->mSpritePosition = position;
}

// TODO: Remove RenderComponents from the list of those who have Animators who have alt views
// when the Animator dequeues an Animation that does not have alt views.
// Also re-add them when the Animator de-queues an Animation that does have alt views.
// To do this we'll need some way to make other systems aware when an Animator has finished one 
// Animation and started another.
void RenderComponent::UpdateAnimatorAltView(const float cameraAngle, const b2Vec2& cameraPosition)
{
	assert(mAnimatorId != AnimatorId::Invalid);

	const b2Vec2 position = GetBody()->GetPosition();
	const b2Vec2 disp = position - cameraPosition;
	const float viewAngle = b2Atan2(disp.y, disp.x) + b2_pi;

	GetAnimationSystem(*this).UpdateAnimatorAltView(
		GetAnimatorId(), 
		GetAnimatorRotation(), 
		viewAngle);
}

// User is responsible for setting the resulting fixture's user data.
b2Fixture* CreateFlatSpriteFixture(b2Body* body, const float halfWidth) {
	assert(body != nullptr);
	assert(body->GetFixtureList() == nullptr);

	b2FixtureDef fdef;
	// Not sure what I'm doing with the filter tbh.
	fdef.filter.categoryBits = 0xF000; // Detached RenderComponent category.
	fdef.filter.maskBits = 0;          // Collide with nothing.
	fdef.filter.groupIndex = -1;       // Never collide with anything. 

	b2EdgeShape shape;
	shape.Set(b2Vec2(-halfWidth, 0.0f), b2Vec2(halfWidth, 0.0f));

	fdef.shape = &shape;

	b2Fixture* newFixture = body->CreateFixture(&fdef);

	return newFixture;
}

void RenderComponent::SetDetached(const bool detached)
{
	if (detached == IsDetached()) return;

	if (detached)
	{
		// Go to detached mode.

		GetFixture()->SetUserData(nullptr);

		{
			b2BodyDef bdef;
			bdef.type = b2BodyType::b2_staticBody;

			mDetachedBody.reset(GetPhysicsWorld(*this)->CreateBody(&bdef));
		}

		{
			b2Fixture* newFixture = CreateFlatSpriteFixture(mDetachedBody.get(), GetSpriteRadius());
			assert(newFixture);

			newFixture->SetUserData(mFixtureRenderData.get());
		}

		GetEntity().GetWorld().RegisterDetachedRenderComponent(*this);
	}
	else
	{
		// Reattach.

		GetEntity().GetWorld().UnregisterDetachedRenderComponent(*this);
		
		mDetachedBody.reset();

		GetFixture()->SetUserData(mFixtureRenderData.get());
	}
}

void RenderComponent::SetSpriteRadius(const float spriteRadius) 
{
	auto log = GetConsoleLogger();

	mFixtureRenderData->mSpriteRadius = spriteRadius;

	if (IsDetached())
	{
		// We can't just change shapes of fixtures after they've been created,
		// so we need to re-create the fixture with the new length.

		// Remove old fixture.
		mDetachedBody->DestroyFixture(mDetachedBody->GetFixtureList());

		// Create new one.
		b2Fixture* newFixture = CreateFlatSpriteFixture(mDetachedBody.get(), GetSpriteRadius());
		newFixture->SetUserData(mFixtureRenderData.get());

		log->debug("RenderComponent::SetSpriteRadius: Recreated fixture for flat sprite.");
	}
}

bool RenderComponent::SetTexture(const std::string& filename)
{
	std::shared_ptr<sf::Texture> texture = GetTextureLibrary(*this).LoadTexture(filename);

	this->mFixtureRenderData->mTexture = texture;

	if (texture)
	{
		this->mTextureFilename = filename;
		
		SetTextureRect(SfVecToRect(GetTexture()->getSize()));

		return true;
	}

	this->mTextureFilename.clear();

	return false;
}

void RenderComponent::RemoveTexture() {
	this->mFixtureRenderData->mTexture = nullptr;
	this->mTextureFilename.clear();
}

void RenderComponent::SetTextureRect(const Animation::Rect& rect)
{
	// If there is no Animator, set the texture rect to the size of the texture.
	// Otherwise leave it; the Animator owns control over the texture rect.
	if (this->mAnimatorId == AnimatorId::Invalid) {
		mFixtureRenderData->mTextureRect.rect = rect;
	}
}

bool RenderComponent::SetAnimation(const AnimationId animationId)
{
	return SetAnimation(animationId, AnimatorRepeatSetting::Forever);
}

bool RenderComponent::SetAnimation(const AnimationId animationId, AnimatorRepeatSetting repeatSetting)
{
	auto log = GetConsoleLogger();

	AnimationSystem& animationSystem = GetAnimationSystem(*this);

	if (!animationSystem.GetAnimations().Contains(animationId)) {
		log->error("There is no Animation with ID {}", animationId);
		return false;
	}

	bool hadImpostors = false;

	if (animationSystem.AnimatorExists(mAnimatorId)) {
		const AnimationId oldAnimation = animationSystem.GetAnimatorAnimation(mAnimatorId);
		hadImpostors = animationSystem.GetAnimations().HasAltViews(oldAnimation);

		if (!animationSystem.SetAnimatorAnimation(
			mAnimatorId,
			AnimatorStartSetting(animationId, repeatSetting)))
		{
			log->error("AnimationSystem::SetAnimatorAnimation failed!");
			return false;
		}
	}
	else {
		mAnimatorId = animationSystem.AddAnimator(
			mFixtureRenderData->mTextureRect,
			AnimatorStartSetting(animationId, repeatSetting));

		if (mAnimatorId == AnimatorId::Invalid) {
			log->error("AnimationSystem::AddAnimator failed!");
			return false;
		}
	}

	if ((!hadImpostors) && animationSystem.GetAnimations().HasAltViews(animationId)) {
		// The old animation did not have impostor frames, but the new one does.
		// Register with the thing.
		if (!GetEntity().GetWorld().RegisterAnimatorWithAltViews(*this)) {
			log->error("RegisterAnimatorWithAltViews failed!");
		}
	}
	else if (hadImpostors) {
		// The old animation had impostor frames, but the new one doesn't.
		// Unregister from the thing.
		if (!GetEntity().GetWorld().UnregisterAnimatorWithAltViews(*this)) {
			log->error("UnregisterAnimatorWithAltViews failed!");
		}
	}

	return true;
}

void RenderComponent::RemoveAnimation()
{
	AnimationSystem& animationSystem = GetAnimationSystem(*this);

	if (!animationSystem.AnimatorExists(mAnimatorId)) return;

	animationSystem.RemoveAnimator(mAnimatorId);

	mAnimatorId = AnimatorId::Invalid;

	// Just do this anyway. Checks are for nerds.
	GetEntity().GetWorld().UnregisterAnimatorWithAltViews(*this);
}

}