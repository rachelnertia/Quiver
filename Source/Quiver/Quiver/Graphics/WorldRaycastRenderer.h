#pragma once

#include <memory>

class b2World;

namespace sf
{
class RenderTarget;
}

namespace qvr {

class Camera3D;
class World;
class WorldRaycastRendererImpl;
struct RenderSettings;

// Takes over the raycasting stage of 3D World rendering from World::Render3D.
class WorldRaycastRenderer
{
public:
	WorldRaycastRenderer();
	~WorldRaycastRenderer();
	void Render(const World& world, const Camera3D& camera, const RenderSettings& settings, sf::RenderTarget& target);
private:
	std::unique_ptr<WorldRaycastRendererImpl> m_Impl;
};

}