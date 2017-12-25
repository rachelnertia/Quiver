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

AnimatorCollection& GetAnimators(const RenderComponent& renderComponent) {
	return renderComponent.GetEntity().GetWorld().GetAnimators();
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
		if (GetEntity().GetWorld().GetAnimators().Remove(mAnimatorId)) {
			log->debug("RenderComponent::Destroy: Removed Animator {}", mAnimatorId);
		}
		else {
			log->error("RenderComponent::Destroy: Remove({}) FAILED!", mAnimatorId);
		}
		mAnimatorId = AnimatorId::Invalid;
	}

	GetFixture()->SetUserData(nullptr);

	if (mDetachedBody) {
		GetEntity().GetWorld().UnregisterDetachedRenderComponent(*this);
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
		const AnimatorCollection& animSystem = GetAnimators(*this);

		if ((mAnimatorId != AnimatorId::Invalid) && animSystem.Exists(mAnimatorId))
		{
			const AnimationId animId = animSystem.GetAnimation(mAnimatorId);

			if (const auto source = animSystem.GetAnimations().GetSourceInfo(animId)) 
			{
				j["Animation"]["File"] = source->filename;
				if (!source->name.empty()) {
					j["Animation"]["Name"] = source->name;
				}
			}

			if (animSystem.GetFrame(mAnimatorId) > 0) {
				j["Animation"]["CurrentFrame"] = animSystem.GetFrame(mAnimatorId);
			}
		}
		else if (GetTexture())
		{
			GetViews().views[0].ToJson(j["TextureRect"]);
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
					SetView(
						mFixtureRenderData->mTextureRects.views, 
						SfVecToRect(GetTexture()->getSize()));
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
			Animation::Rect singleView;
			singleView.FromJson(j["TextureRect"]);

			SetView(mFixtureRenderData->mTextureRects.views, singleView);
		}
	}

	if (FieldExistsInJson("Animation", j)) {
		const AnimationSourceInfo animSource = j["Animation"];
		
		if (!animSource.filename.empty()) {
			AnimatorCollection& animSystem = GetEntity().GetWorld().GetAnimators();

			AnimationId animId = animSystem.GetAnimations().GetAnimation(animSource);

			if (animId != AnimationId::Invalid) {
				if (SetAnimation(animId)) {
					if (FieldExistsInJson("CurrentFrame", j["Animation"])) {
						const unsigned currentFrame = j["Animation"]["CurrentFrame"].get<unsigned>();

						animSystem.SetFrame(mAnimatorId, currentFrame);
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
		SetView(mFixtureRenderData->mTextureRects.views, rect);
	}
}

bool RenderComponent::SetAnimation(const AnimationId animationId)
{
	return SetAnimation(animationId, AnimatorRepeatSetting::Forever);
}

bool RenderComponent::SetAnimation(const AnimationId animationId, AnimatorRepeatSetting repeatSetting)
{
	auto log = GetConsoleLogger();

	AnimatorCollection& animators = GetAnimators(*this);

	if (!animators.GetAnimations().Contains(animationId)) {
		log->error("There is no Animation with ID {}", animationId);
		return false;
	}

	if (animators.Exists(mAnimatorId)) {
		const AnimationId oldAnimation = animators.GetAnimation(mAnimatorId);

		if (!animators.SetAnimation(
			mAnimatorId,
			AnimatorStartSetting(animationId, repeatSetting)))
		{
			log->error("AnimatorCollection::SetAnimation failed!");
			return false;
		}
	}
	else {
		mAnimatorId = animators.Add(
			mFixtureRenderData->mTextureRects,
			AnimatorStartSetting(animationId, repeatSetting));

		if (mAnimatorId == AnimatorId::Invalid) {
			log->error("AnimatorCollection::Add failed!");
			return false;
		}
	}

	return true;
}

void RenderComponent::RemoveAnimation()
{
	AnimatorCollection& animators = GetAnimators(*this);

	if (!animators.Exists(mAnimatorId)) return;

	animators.Remove(mAnimatorId);

	mAnimatorId = AnimatorId::Invalid;
}

}