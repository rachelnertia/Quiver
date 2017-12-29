#pragma once

#include "SFML/System/Clock.hpp"

#include "Quiver/Application/ApplicationState.h"
#include "Quiver/Graphics/Camera2D.h"
#include "Quiver/Graphics/Camera3D.h"
#include "Quiver/Graphics/WorldRaycastRenderer.h"
#include "Quiver/Input/SfmlJoystick.h"
#include "Quiver/Input/SfmlKeyboard.h"
#include "Quiver/Input/SfmlMouse.h"

namespace sf {
class RenderTexture;
}

namespace qvr {

class World;

class Game : public ApplicationState {
public:
	Game(ApplicationStateContext& context);
	Game(ApplicationStateContext& context, std::unique_ptr<World> world);

	~Game();

	Game(const Game&) = delete;
	Game(const Game&&) = delete;

	Game& operator=(const Game&) = delete;
	Game& operator=(const Game&&) = delete;

	void ProcessFrame() override;

private:
	void OnTogglePause();
	void ProcessGui();

	bool mCamera2DFollowCamera3D = true;
	bool mDrawOverhead = false;

	nlohmann::json mWorldJson;

	std::unique_ptr<World> mWorld;

	WorldRaycastRenderer mWorldRaycastRenderer;

	Camera2D mCamera2D;

	Camera3D mDefaultCamera3D;

	sf::Clock mFrameClock;

	float mFrameTime = 0.0f;

	float mFrameTexResolutionModifier = 1.0f;

	std::unique_ptr<sf::RenderTexture> mFrameTex;

	bool mPaused = false;

	qvr::SfmlJoystickSet mJoysticks;
	qvr::SfmlKeyboard mKeyboard;
	qvr::SfmlMouse mMouse;
};

}