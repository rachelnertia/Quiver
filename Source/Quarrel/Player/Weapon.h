#pragma once

struct b2Vec2;

namespace sf {
class RenderTarget;
class Color;
}

namespace qvr {
class Entity;
class RawInputDevices;
class World;
}

class Player;

class Weapon {
protected:
	Player& mPlayer;
public:
	Weapon(Player& player) : mPlayer(player) {};

	virtual ~Weapon() = default;

	virtual void HandleInput(
		const qvr::RawInputDevices& inputDevices,
		const float deltaSeconds) = 0;

	virtual void Render(sf::RenderTarget& target) {}
};
