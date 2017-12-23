#pragma once

#include <memory>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Texture.hpp>

#include "Quiver/Animation/Animators.h"

namespace qvr
{

class RenderComponent;

class FixtureRenderData
{
	friend class RenderComponent;

	float mHeight = 1.0f;
	float mGroundOffset = 0.0f;
	float mSpriteRadius = 0.5f;
	float mObjectAngle = 0.0f;
	b2Vec2 mSpritePosition;

	sf::Color mBlendColor = sf::Color(255, 255, 255, 255);

	std::shared_ptr<sf::Texture> mTexture;

	AnimatorTarget mTextureRects;

public:
	float GetHeight() const { return mHeight; }
	float GetGroundOffset() const { return mGroundOffset; }
	float GetSpriteRadius() const { return mSpriteRadius; }
	float GetObjectAngle() const { return mObjectAngle; }

	const b2Vec2& GetSpritePosition() const { return mSpritePosition; }

	sf::Color GetColor() const { return mBlendColor; }

	const sf::Texture* GetTexture() const { return mTexture.get(); }

	const ViewBuffer& GetViews() const { return mTextureRects.views; }
};

}