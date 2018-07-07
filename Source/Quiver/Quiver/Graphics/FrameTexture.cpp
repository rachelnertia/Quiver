#include "FrameTexture.h"

#include <SFML/Graphics/RenderTexture.hpp>

namespace qvr {

void UpdateFrameTexture(
	sf::RenderTexture& texture,
	sf::Vector2u const& windowDimensions,
	float const ratio)
{
	unsigned const x = unsigned(windowDimensions.x * ratio);
	unsigned const y = unsigned(windowDimensions.y * ratio);

	if (texture.getSize() != sf::Vector2u(x, y)) {
		texture.create(
			unsigned(windowDimensions.x * ratio),
			unsigned(windowDimensions.y * ratio));
	}
}

}