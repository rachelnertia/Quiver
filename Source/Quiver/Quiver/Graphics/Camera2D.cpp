#include "Camera2D.h"

#include <algorithm>

#include <SFML/Window/Keyboard.hpp>

namespace qvr
{

void FreeControl(Camera2D& camera, float const dt)
{
	float movespeed = 125.0f / camera.mPixelsPerMetre;

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) {
		b2Vec2 dir = b2Mul(camera.mTransform.q, b2Vec2(0.0f, -1.0f));
		camera.MoveBy(movespeed * dt * dir);
	}
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) {
		b2Vec2 dir = b2Mul(camera.mTransform.q, b2Vec2(0.0f, 1.0f));
		camera.MoveBy(movespeed * dt * dir);
	}
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) {
		b2Vec2 dir = b2Mul(camera.mTransform.q, b2Vec2(-1.0f, 0.0f));
		camera.MoveBy(movespeed * dt * dir);
	}
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) {
		b2Vec2 dir = b2Mul(camera.mTransform.q, b2Vec2(1.0f, 0.0f));
		camera.MoveBy(movespeed * dt * dir);
	}

	float turnspeed = 1.5f;

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::E)) {
		camera.RotateBy(-turnspeed * dt);
	}
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Q)) {
		camera.RotateBy(turnspeed * dt);
	}

	float zoomspeed = 10.0f;

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::R)) {
		camera.mPixelsPerMetre += zoomspeed * dt;
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::F)) {
		camera.mPixelsPerMetre -= zoomspeed * dt;
		camera.mPixelsPerMetre = std::max(camera.mPixelsPerMetre, 0.0f);
	}
}

auto GetFreeControlCamera2DInstructions() -> const char*
{
	return
		"Move:   WASD \n"
		"Rotate: Q and E \n"
		"Zoom:   R and F";
}

}