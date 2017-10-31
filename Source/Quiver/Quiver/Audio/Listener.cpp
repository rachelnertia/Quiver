#include "Listener.h"

#include <Box2D/Common/b2Math.h>
#include <SFML/Audio/Listener.hpp>

#include "Quiver/Graphics/Camera3D.h"

namespace qvr
{

void UpdateListener(const b2Vec2& position, const b2Vec2& direction) {
	sf::Listener::setPosition(position.x, 0.0f, position.y);
	sf::Listener::setDirection(direction.x, 0.0f, direction.y);
	sf::Listener::setUpVector(0.0f, 1.0f, 0.0f);
}

void UpdateListener(const Camera3D& camera) {
	UpdateListener(camera.GetPosition(), camera.GetForwards());
}

}