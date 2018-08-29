#pragma once

#include <functional>

namespace sf {
class RenderTarget;
}

namespace qvr {

class World;

// Represents a thing in the World that adds GUI elements to the screen.
// For example, a thing that draws the player's HUD.
class WorldUiRenderer final {
public:
	using RenderFunction = std::function<void(sf::RenderTarget&)>;

	WorldUiRenderer(World& world, RenderFunction renderFunction);
	~WorldUiRenderer();

	void Render(sf::RenderTarget& target) {
		renderFunction(target);
	}

private:
	World & world;
	RenderFunction renderFunction;
};

}