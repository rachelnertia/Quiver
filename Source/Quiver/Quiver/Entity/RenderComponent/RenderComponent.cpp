#include "RenderComponent.h"

#include <Box2D/Collision/Shapes/b2EdgeShape.h>
#include <Box2D/Dynamics/b2Body.h>
#include <Box2D/Dynamics/b2Fixture.h>
#include <Box2D/Dynamics/b2World.h>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Shader.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <spdlog/spdlog.h>

#include "Quiver/Entity/Entity.h"
#include "Quiver/Graphics/Camera3D.h"
#include "Quiver/Graphics/ColourUtils.h"
#include "Quiver/Graphics/Light.h"
#include "Quiver/Graphics/TextureLibrary.h"
#include "Quiver/Entity/PhysicsComponent/PhysicsComponent.h"
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
	if (mAnimatorId != AnimatorId::Invalid) {
		if (GetEntity().GetWorld().GetAnimationSystem().RemoveAnimator(mAnimatorId)) {
			std::cout << "RenderComponent::Destroy: Removed Animator #" << mAnimatorId << "\n";
		}
		else {
			std::cout << "RenderComponent::Destroy: RemoveAnimator(" << mAnimatorId << ") FAILED! \n";
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

			AnimationSourceInfo animSource;
			if (animSystem.GetAnimationSourceInfo(animId, animSource)) {
				j["Animation"]["File"] = animSource.filename;
				if (!animSource.name.empty()) {
					j["Animation"]["Name"] = animSource.name;
				}
			}

			if (animSystem.GetAnimatorFrame(mAnimatorId) > 0) {
				j["Animation"]["CurrentFrame"] = animSystem.GetAnimatorFrame(mAnimatorId);

				std::cout << "j[\"Animation\"][\"CurrentFrame\"] == " << j["Animation"]["CurrentFrame"].get<unsigned>() << '\n';
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
			std::cout << "Texture field must be a filename (string)." << std::endl;
		}

		if (FieldExistsInJson("TextureRect", j))
		{
			mFixtureRenderData->mTextureRect.rect.FromJson(j["TextureRect"]);
		}
	}

	if (FieldExistsInJson("Animation", j)) {
		if (FieldExistsInJson("File", j["Animation"])) {
			AnimationSourceInfo animSource;

			animSource.filename = j["Animation"]["File"].get<std::string>();

			if (FieldExistsInJson("Name", j["Animation"])) {
				animSource.name = j["Animation"]["Name"].get<std::string>();
			}

			AnimationSystem& animSystem = GetEntity().GetWorld().GetAnimationSystem();

			AnimationId animId = animSystem.GetAnimationFromSource(animSource);

			// TODO: Load AnimationData from the AnimationSourceInfo and add it to the AnimationSystem.
			// Or handle that in GetAnimationFromSource or something.

			if (animSystem.AnimationExists(animId) &&
				SetAnimation(animId))
			{
				if (FieldExistsInJson("CurrentFrame", j["Animation"])) {
					const unsigned currentFrame = j["Animation"]["CurrentFrame"].get<unsigned>();

					std::cout << "j[\"Animation\"][\"CurrentFrame\"] == " << currentFrame << "\n";

					animSystem.SetAnimatorFrame(mAnimatorId, currentFrame);
				}
			}
		}
	}

	// All done!

	return true;
}

bool RenderComponent::VerifyJson(const nlohmann::json & j)
{
	if (j.empty()) {
		std::cout << "JSON object is empty!" << std::endl;
		return false;
	}

	auto VerifyFieldIsPresent = [](std::string name, const nlohmann::json & j) -> bool {
		if (j.find(name) != j.end()) {
			return true;
		}
		std::cout << name << " field missing!" << std::endl;
		return false;
	};

	if (j.count("Height") && !j["Height"].is_number())       return false;
	if (j.count("GroundOffset") && !j["GroundOffset"].is_number()) return false;

	if (j.count("Colour") && !ColourUtils::VerifyColourJson(j["Colour"])) return false;
	
	if (j.find("Texture") != j.end()) {
		if (!j["Texture"].is_string()) {
			std::cout << "Texture field must be a filename (string)." << std::endl;
		}
	}

	if (j.find("SpriteRadius") != j.end()) {
		if (!j["SpriteRadius"].is_number()) {
			std::cout << "SpriteRadius field must be a number." << std::endl;
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

		std::cout << "RenderComponent::SetSpriteRadius: Recreated fixture for flat sprite.\n";
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
	AnimationSystem& animationSystem = GetAnimationSystem(*this);

	if (!animationSystem.AnimationExists(animationId)) {
		std::cout << "There is no Animation with ID " << animationId << "!\n";
		return false;
	}

	bool hadImpostors = false;

	if (animationSystem.AnimatorExists(mAnimatorId)) {
		const AnimationId oldAnimation = animationSystem.GetAnimatorAnimation(mAnimatorId);
		hadImpostors = animationSystem.AnimationHasAltViews(oldAnimation);

		if (!animationSystem.SetAnimatorAnimation(
			mAnimatorId,
			AnimatorStartSetting(animationId, repeatSetting)))
		{
			std::cout << "AnimationSystem::SetAnimatorAnimation failed!\n";
			return false;
		}
	}
	else {
		mAnimatorId = animationSystem.AddAnimator(
			mFixtureRenderData->mTextureRect,
			AnimatorStartSetting(animationId, repeatSetting));

		if (mAnimatorId == AnimatorId::Invalid) {
			std::cout << "AnimationSystem::AddAnimator failed!\n";
			return false;
		}
	}

	if ((!hadImpostors) && animationSystem.AnimationHasAltViews(animationId)) {
		// The old animation did not have impostor frames, but the new one does.
		// Register with the thing.
		if (!GetEntity().GetWorld().RegisterAnimatorWithAltViews(*this)) {
			std::cout << "RegisterAnimatorWithAltViews failed!" << std::endl;
		}
	}
	else if (hadImpostors) {
		// The old animation had impostor frames, but the new one doesn't.
		// Unregister from the thing.
		if (!GetEntity().GetWorld().UnregisterAnimatorWithAltViews(*this)) {
			std::cout << "UnregisterAnimatorWithAltViews failed!" << std::endl;
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