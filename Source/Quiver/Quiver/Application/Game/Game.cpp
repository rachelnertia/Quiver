#include "Game.h"

#include <SFML/Audio/Listener.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Window/Event.hpp>
#include <ImGui/imgui.h>
#include <ImGui/imgui-SFML.h>
#include <spdlog/spdlog.h>

#include "Quiver/Application/WorldEditor/WorldEditor.h"
#include "Quiver/Input/RawInput.h"
#include "Quiver/Input/InputDebug.h"
#include "Quiver/Misc/ImGuiHelpers.h"
#include "Quiver/World/World.h"
#include "Quiver/World/WorldContext.h"

namespace qvr {

Game::Game(ApplicationStateContext& context)
	: Game(context, nullptr)
{}

Game::Game(ApplicationStateContext& context, std::unique_ptr<World> world)
	: ApplicationState(context)
	, mWorld(world.release())
	, mMouse(context.GetWindow())
	, mFrameTex(std::make_unique<sf::RenderTexture>())
{
	if (!mWorld) {
		mWorld.reset(new World(GetContext().GetWorldContext()));
	}

	// Save the World-state so we can rollback to it.
	mWorld->ToJson(mWorldJson);

	const sf::Vector2u windowSize = GetContext().GetWindow().getSize();

	mFrameTex->create(
		unsigned(windowSize.x * GetContext().GetFrameTextureResolutionRatio()),
		unsigned(windowSize.y * GetContext().GetFrameTextureResolutionRatio()));

	mFrameClock.restart();

	mCamera2D.mOffsetX = (float)mFrameTex->getSize().x / 2.0f;
	mCamera2D.mOffsetY = (float)mFrameTex->getSize().y / 2.0f;
}

Game::~Game() {}

void Game::ProcessFrame()
{
	using namespace std::chrono_literals;

	if (GetContext().WindowResized()) {
		const auto newSize = GetContext().GetWindow().getSize();
		mFrameTex->create(newSize.x, newSize.y);
	}

	// Clamp excessively large delta times.
	const float delta = std::min(mFrameClock.restart().asSeconds(), 1.0f / 30.0f);

	mTimeSinceLastStep += std::chrono::duration<float>(delta);

	// Use free camera controls if the World doesn't have a 'main' camera currently.
	if ((mWorld->GetMainCamera() == nullptr) && GetContext().GetWindow().hasFocus())
	{
		FreeControl(mDefaultCamera3D, delta);
	}

	if (mCamera2DFollowCamera3D)
	{
		const Camera3D& currentCamera3D = mWorld->GetMainCamera() ? *mWorld->GetMainCamera() : mDefaultCamera3D;
		mCamera2D.SetPosition(currentCamera3D.GetPosition());
	}

	const auto timestep = mWorld->GetTimestep();

	// Take a step if 1/60th of a second has passed.
	if (mTimeSinceLastStep >= timestep)
	{
		mTimeSinceLastStep = 0.0s;

		mMouse.OnStep();
		mKeyboard.Update();
		mJoysticks.Update();

		if (mKeyboard.JustDown(qvr::KeyboardKey::Escape)) {
			mPaused = !mPaused;
			OnTogglePause();
		}

		if (!mPaused) {
			qvr::RawInputDevices devices(mMouse, mKeyboard, mJoysticks);

			mWorld->TakeStep(devices);
		}

		// Render World:
		{
			mFrameTex->clear(sf::Color(128, 128, 255));

			mWorld->Render3D(
				*mFrameTex,
				mWorld->GetMainCamera() ? *mWorld->GetMainCamera() : mDefaultCamera3D,
				mWorldRaycastRenderer);

			mFrameTex->display();
		}
	}

	mMouse.OnFrame();

	GetContext().GetWindow().clear(sf::Color::Black);

	// Copy the frame texture to the window.
	{
		sf::RenderWindow& window = GetContext().GetWindow();
		sf::Sprite frameSprite(mFrameTex->getTexture());
		frameSprite.setScale(
			(float)window.getSize().x / (float)mFrameTex->getSize().x,
			(float)window.getSize().y / (float)mFrameTex->getSize().y);
		window.draw(frameSprite);
	}

	// Draw the overhead overlay here so its resolution is the same as the Window, not the 
	// Render3D target texture.
	if (mDrawOverhead) {
		mWorld->RenderDebug(GetContext().GetWindow(), mCamera2D);
	}

	ProcessGui();

	// WorldRaycastRenderer messes up the SFML GL state.
	// Need this so that we can have both ImGui-SFML and WorldRaycastRenderer.
	// I don't quite understand it.
	GetContext().GetWindow().resetGLStates();

	ImGui::Render();
	
	GetContext().GetWindow().display();

	if (mWorld->GetNextWorld())
	{
		mWorld = std::move(mWorld->GetNextWorld());
	}
	
	{
		auto applicationState = mWorld->GetNextApplicationState(GetContext());
		if (applicationState) {
			SetQuit(std::move(applicationState));
		}
	}
}

void Game::OnTogglePause()
{
	auto log = spdlog::get("console");
	assert(log);

	GetContext().GetWindow().setMouseCursorVisible(true);

	mWorld->SetPaused(mPaused);

	if (mPaused) {
		// Game has just been paused.
		mMouse.SetHidden(false);
		mMouse.SetMouselook(false);

		log->debug("Paused");
	}
	else {
		// Game has just been unpaused.

		log->debug("Unpaused");
	}
}

void Game::ProcessGui()
{
	ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
	ImGui::SetNextWindowSize(ImVec2(600.0f, (float)GetContext().GetWindow().getSize().y));

	ImGui::AutoWindow window("Game", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

	ImGui::AutoStyleVar styleVar(ImGuiStyleVar_IndentSpacing, 5.0f);

	if (ImGui::Checkbox("Paused", &mPaused)) {
		OnTogglePause();
	}

	if (ImGui::Button("Edit!")) {
		std::unique_ptr<World> newWorld;
		
		try
		{
			// Reload the World back to the state it was in when we entered Game mode.
			newWorld = std::make_unique<World>(GetContext().GetWorldContext(), mWorldJson);
		}
		catch (std::exception)
		{
		}

		SetQuit(std::make_unique<WorldEditor>(GetContext(), std::move(newWorld)));

		return;
	}

	if (ImGui::Button("Restart!")) {
		try
		{
			// Reload the World back to the state it was in when we entered Game mode.
			auto newWorld = std::make_unique<World>(GetContext().GetWorldContext(), mWorldJson);

			mWorld.swap(newWorld);
		}
		catch (std::exception e)
		{
			mWorld = std::make_unique<World>(GetContext().GetWorldContext());
		}
	}

	if (ImGui::CollapsingHeader("Options")) {
		ImGui::AutoIndent indent;
		{
			if (ImGui::SliderFloat("Horizontal Resolution", &GetContext().GetFrameTextureResolutionRatio(), 0.2f, 1.0f)) {
				sf::Vector2u windowSize = GetContext().GetWindow().getSize();
				mFrameTex->create(
					unsigned(windowSize.x * GetContext().GetFrameTextureResolutionRatio()), 
					unsigned(windowSize.y * GetContext().GetFrameTextureResolutionRatio()));
			}
		}
		{
			float currentVolume = sf::Listener::getGlobalVolume();
			if (ImGui::SliderFloat("Global Volume", &currentVolume, 0.0f, 100.0f)) {
				sf::Listener::setGlobalVolume(currentVolume);
			}
		}
	}

	if (ImGui::CollapsingHeader("Performance Info")) {
		ImGui::AutoIndent indent;
		{
			ImGui::Text("Application FPS: %.f", ImGui::GetIO().Framerate);

			if (ImGui::CollapsingHeader("World")) {
				mWorld->GuiPerformanceInfo();
			}
		}
	}

	if (ImGui::CollapsingHeader("Overhead Overlay")) {
		ImGui::AutoIndent indent;
		{
			ImGui::Checkbox("Draw 2D Overlay", &mDrawOverhead);
			ImGui::Checkbox("Camera 2D Follows Main Camera", &mCamera2DFollowCamera3D);
		}
	}

	if (ImGui::CollapsingHeader("Joystick Debug Window")) {
		ImGui::AutoWindow joystickDebugWindow("Joystick Debug", nullptr, 0);

		SfmlJoystickDebugGui(this->mJoysticks);
	}
}

}