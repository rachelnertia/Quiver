#pragma once

#include <SFML/System/Vector2.hpp>

// Intended to be used in ApplicationStates that use pseudo-3d rendering

namespace sf {
class RenderTexture;
}

namespace qvr {

void UpdateFrameTexture(
	sf::RenderTexture& texture,
	sf::Vector2u const& windowDimensions,
	float const ratio);

}