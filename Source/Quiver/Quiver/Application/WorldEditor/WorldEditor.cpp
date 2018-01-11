#include "WorldEditor.h"

#include <iostream>
#include <string>
#include <fstream>

#include <Box2D/Collision/Shapes/b2CircleShape.h>
#include <Box2D/Collision/Shapes/b2PolygonShape.h>
#include <Box2D/Dynamics/b2Fixture.h>
#include <Box2D/Dynamics/b2World.h>
#include <Box2D/Dynamics/b2WorldCallbacks.h>
#include <ImGui/imgui.h>
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Window/Event.hpp>
#include <spdlog/spdlog.h>

#include "Quiver/Application/Game/Game.h"
#include "Quiver/Entity/Entity.h"
#include "Quiver/Entity/EntityEditor.h"
#include "Quiver/Entity/PhysicsComponent/PhysicsComponent.h"
#include "Quiver/Entity/RenderComponent/RenderComponent.h"
#include "Quiver/Graphics/ColourUtils.h"
#include "Quiver/Graphics/TextureLibrary.h"
#include "Quiver/Misc/ImGuiHelpers.h"
#include "Quiver/World/World.h"

#include "EditorTools.h"

namespace qvr {

WorldEditor::WorldEditor(ApplicationStateContext & context)
	: WorldEditor(context, nullptr)
{}

WorldEditor::WorldEditor(ApplicationStateContext& context, std::unique_ptr<World> world)
	: ApplicationState(context)
	, mWorld(std::move(world))
	, mFrameTex(std::make_unique<sf::RenderTexture>())
	, mMouse(context.GetWindow())
{
	if (!mWorld) {
		mWorld = std::make_unique<World>(GetContext().GetWorldContext());
	}

	// Instantiate EditorTools.
	mTools.push_back(std::make_unique<NoTool>());
	mTools.push_back(std::make_unique<SelectTool>());
	mTools.push_back(std::make_unique<CopySelectedEntityTool>());
	mTools.push_back(std::make_unique<MoveTool>());
	mTools.push_back(std::make_unique<RotateTool>());
	mTools.push_back(std::make_unique<CreateCircleTool>());
	mTools.push_back(std::make_unique<CreateBoxTool>());
	mTools.push_back(std::make_unique<CreatePolygonTool>());
	mTools.push_back(std::make_unique<CreateInstanceOfPrefabTool>());

	mCurrentToolIndex = 1;

	const sf::Vector2u windowSize = GetContext().GetWindow().getSize();

	mFrameTex->create(
		unsigned(windowSize.x * mFrameTexResolutionModifier),
		unsigned(windowSize.y * mFrameTexResolutionModifier));
}

WorldEditor::~WorldEditor() {}

void WorldEditor::ProcessFrame()
{
	if (GetContext().WindowResized()) {
		const auto newSize = GetContext().GetWindow().getSize();
		mFrameTex->create(newSize.x, newSize.y);
	}

	auto dt = frameClock.restart();

	HandleInput(dt.asSeconds());

	ProcessGUI();

	mAnimationEditor.Update(dt.asSeconds());

	if (GetQuit()) {
		ImGui::EndFrame();
		return;
	}

	Render();
}

void WorldEditor::HandleInput(const float dt)
{
	mJoysticks.Update();
	mKeyboard.Update();
	mMouse.Update();

	if (mMouse.GetPositionDelta() != sf::Vector2i(0, 0)) {
		OnMouseMove(
			sf::Event::MouseMoveEvent{ 
				mMouse.GetPositionRelative().x, 
				mMouse.GetPositionRelative().y });
	}

	if (mMouse.JustDown(qvr::MouseButton::Left)) {
		OnMouseClick(
			sf::Event::MouseButtonEvent{
				sf::Mouse::Button::Left,
				mMouse.GetPositionRelative().x,
				mMouse.GetPositionRelative().y });
	}

	if (!GetContext().GetWindow().hasFocus()) {
		return;
	}

	if (ImGui::GetIO().WantTextInput) {
		return;
	}

	if (mInPreviewMode) {
		FreeControl(mCamera.Get3D(), dt);

		mCamera.Get2D().SetPosition(mCamera.Get3D().GetPosition());
		mCamera.Get2D().SetRotation(mCamera.Get3D().GetRotation() - b2_pi);

	}
	else {
		float movespeed = 125.0f / mCamera.Get2D().mPixelsPerMetre;

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) {
			b2Vec2 dir = b2Mul(mCamera.Get2D().mTransform.q, b2Vec2(0.0f, -1.0f));
			mCamera.Get2D().MoveBy(movespeed * dt * dir);
		}
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) {
			b2Vec2 dir = b2Mul(mCamera.Get2D().mTransform.q, b2Vec2(0.0f, 1.0f));
			mCamera.Get2D().MoveBy(movespeed * dt * dir);
		}
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) {
			b2Vec2 dir = b2Mul(mCamera.Get2D().mTransform.q, b2Vec2(-1.0f, 0.0f));
			mCamera.Get2D().MoveBy(movespeed * dt * dir);
		}
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) {
			b2Vec2 dir = b2Mul(mCamera.Get2D().mTransform.q, b2Vec2(1.0f, 0.0f));
			mCamera.Get2D().MoveBy(movespeed * dt * dir);
		}

		float turnspeed = 1.5f;

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::E)) {
			mCamera.Get2D().RotateBy(-turnspeed * dt);
		}
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Q)) {
			mCamera.Get2D().RotateBy(turnspeed * dt);
		}

		mCamera.Get3D().SetPosition(mCamera.Get2D().GetPosition());
		mCamera.Get3D().SetRotation(mCamera.Get2D().GetRotation() + b2_pi);

		float zoomspeed = 10.0f;

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::R)) {
			mCamera.Get2D().mPixelsPerMetre += zoomspeed * dt;
		}

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::F)) {
			mCamera.Get2D().mPixelsPerMetre -= zoomspeed * dt;
			mCamera.Get2D().mPixelsPerMetre = std::max(mCamera.Get2D().mPixelsPerMetre, 0.0f);
		}
	}
}

void WorldEditor::Render()
{
	sf::RenderWindow& window = GetContext().GetWindow();

	window.clear(sf::Color(128, 128, 255));

	if (mInPreviewMode) {
		mFrameTex->clear(sf::Color(128, 128, 255));

		mWorld->Render3D(*mFrameTex, mCamera.Get3D(), mWorldRaycastRenderer);

		mFrameTex->display();

		// Copy the frame texture to the window.
		{
			sf::Sprite frameSprite(mFrameTex->getTexture());
			frameSprite.setScale((float)window.getSize().x / (float)mFrameTex->getSize().x,
				(float)window.getSize().y / (float)mFrameTex->getSize().y);
			window.draw(frameSprite);
		}
	}
	else {
		mCamera.Get2D().mOffsetX = (float)window.getSize().x / 2.0f;
		mCamera.Get2D().mOffsetY = (float)window.getSize().y / 2.0f;

		mWorld->RenderDebug(window, mCamera.Get2D());

		mTools.at(mCurrentToolIndex)->Draw(window, mCamera.Get2D());
	}

	// Mark the middle of the screen
	{
		sf::RectangleShape r;
		r.setPosition(sf::Vector2f((float)window.getSize().x / 2.0f, (float)window.getSize().y / 2.0f));
		r.setSize(sf::Vector2f(3.0f, 3.0f));
		r.setOrigin(sf::Vector2f(1.5f, 1.5f));
		r.setFillColor(sf::Color::White);
		window.draw(r);
	}

	ImGui::Render();

	window.display();
}

void WorldEditor::ProcessGUI()
{
	auto log = spdlog::get("console");
	assert(log);

	ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
	ImGui::SetNextWindowSize(ImVec2(600.0f, (float)GetContext().GetWindow().getSize().y));

	ImGui::AutoWindow window("World Editor", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

	ImGui::AutoStyleVar styleVar(ImGuiStyleVar_IndentSpacing, 5.0f);

	if (ImGui::Button("Play!")) {
		SetQuit(std::make_unique<Game>(GetContext(), std::move(mWorld)));
		return;
	}

	if (ImGui::Button("New")) {
		mWorld = std::make_unique<World>(GetContext().GetWorldContext());
		mCurrentSelection = nullptr;
		mWorldFilename.clear();
	}

	if (ImGui::CollapsingHeader("Save/Load")) {
		ImGui::InputText<64>("Filename", mWorldFilename);

		if (ImGui::Button("Save")) {
			if (mWorldFilename.empty()) {
				log->error("No filename specified.");
			}
			else {
				SaveWorld(*mWorld, mWorldFilename);
			}
		}

		if (ImGui::Button("Load")) {
			if (mWorldFilename.empty()) {
				log->error("No filename specified.");
			}
			else {
				mCurrentSelection = nullptr;

				if (auto newWorld =
					LoadWorld(
						mWorldFilename,
						GetContext().GetWorldContext()))
				{
					mWorld.reset(newWorld.release());
				}
			}
		}
	}

	if (ImGui::CollapsingHeader("Animation Editor")) {
		if (ImGui::Button("Open/Close Animation Editor")) {
			mAnimationEditor.SetOpen(!mAnimationEditor.IsOpen());
		}

		if (mAnimationEditor.IsOpen()) {
			mAnimationEditor.ProcessGui();
		}
	}

	if (ImGui::CollapsingHeader("Options")) {
		ImGui::AutoIndent indent;
		
		if (ImGui::SliderFloat("Resolution Thing", &mFrameTexResolutionModifier, 0.2f, 1.0f)) {
			sf::Vector2u windowSize = GetContext().GetWindow().getSize();
			mFrameTex->create(unsigned(windowSize.x * mFrameTexResolutionModifier), unsigned(windowSize.y * mFrameTexResolutionModifier));
		}
	}

	if (ImGui::CollapsingHeader("Camera")) {
		if (ImGui::Button(mInPreviewMode ? "Switch to 2D" : " Switch to 3D")) {
			mInPreviewMode = !mInPreviewMode;
		}

		{
			b2Vec2 p = mCamera.Get2D().GetPosition();
			if (ImGui::InputFloat2("Position", &p.x)) {
				mCamera.SetPosition(p);
			}
		}
		{
			float angle = mCamera.Get2D().GetRotation();
			if (ImGui::SliderAngle("Rotation", &angle, -180.0f, 180.0f)) {
				mCamera.SetRotation(angle);
			}
		}

		if (mInPreviewMode) {
			float height = mCamera.Get3D().GetHeight();
			if (ImGui::SliderFloat("Camera Height", &height, 0.0f, 4.0f)) {
				mCamera.Get3D().SetHeight(height);
			}

			float pitchRadians = mCamera.Get3D().GetPitchRadians();
			if (ImGui::SliderAngle("Pitch", &pitchRadians, -45.0f, 45.0f)) {
				mCamera.Get3D().SetPitch(pitchRadians);
			}

			float fovRadians = mCamera.Get3D().GetFovRadians();
			if (ImGui::SliderAngle("Horizontal FOV", &fovRadians, 45.0f, 135.0f)) {
				mCamera.Get3D().SetFov(fovRadians);
			}
		}

		if (!mInPreviewMode) {
			float pixelsPerMetre = mCamera.Get2D().mPixelsPerMetre;
			if (ImGui::SliderFloat("Pixels Per Metre", &pixelsPerMetre, 1.0f, 128.0f)) {
				mCamera.Get2D().mPixelsPerMetre = pixelsPerMetre;
			}
		}
	}

	if (ImGui::CollapsingHeader("Performance Info")) {
		ImGui::AutoIndent indent;
		
		ImGui::Text("Application FPS: %.f", ImGui::GetIO().Framerate);

		if (ImGui::CollapsingHeader("World##perf")) {
			mWorld->GuiPerformanceInfo();
		}
	}

	if (ImGui::CollapsingHeader("World##ctrl")) {
		ImGui::AutoIndent indent;

		mWorld->GuiControls();
	}

	if (ImGui::CollapsingHeader("World Animation System")) {
		ImGui::AutoIndent indent;

		GuiControls(mWorld->GetAnimators(), mAnimationLibraryEditorData);
	}

	if (ImGui::CollapsingHeader("Texture Library")) {
		ImGui::AutoIndent indent;

		if (mTextureLibraryGui == nullptr) {
			mTextureLibraryGui =
				std::make_unique<TextureLibraryGui>(mWorld->GetTextureLibrary());
		}

		mTextureLibraryGui->ProcessGui();
	}

	if (ImGui::CollapsingHeader("Editor Tools")) {
		// Create a list, with selectable entries, of all the EditorTools in mTools.
		// The strings to populate the list with are extracted from mTools using the
		// anonymous lambda (3rd argument to ListBox).
		int previousIndex = mCurrentToolIndex;
		bool selectionChanged = ImGui::ListBox("Tool Select", &mCurrentToolIndex,
			[](void* data, int index, const char** itemText) -> bool {

			auto toolArray = (std::vector<EditorTool*>*)data;

			if (index >= (int)toolArray->size()) return false;
			if (index < 0) return false;

			if (toolArray->at(index) == nullptr) return false;

			*itemText = toolArray->at(index)->GetName();

			return true;

		}, (void*)&mTools, (int)mTools.size());
		if (selectionChanged) {
			mTools.at(previousIndex)->OnCancel(*this, mCamera.Get2D());
		}

		mTools.at(mCurrentToolIndex)->DoGui(*this);
	}

	if (ImGui::CollapsingHeader("Selected Entity")) {
		ImGui::AutoIndent indent;

		if (mCurrentSelection) {
			if (ImGui::Button("Delete Entity")) {
				mWorld->RemoveEntityImmediate(*mCurrentSelection);
				mCurrentSelection = nullptr;
				mCurrentSelectionEditor.release();
			}
			else {
				// Make a new EntityEditor if the old one is invalid.
				if (!mCurrentSelectionEditor ||
					!mCurrentSelectionEditor->IsTargeting(*mCurrentSelection))
				{
					mCurrentSelectionEditor = std::make_unique<EntityEditor>(*mCurrentSelection);
				}
				// Edit the Entity!
				mCurrentSelectionEditor->GuiControls();
			}
		}
		else {
			ImGui::Text("No Entity is selected. Use the Select Tool to pick one!");
		}
	}
}

void WorldEditor::OnMouseClick(const sf::Event::MouseButtonEvent & mouseInfo)
{
	if (mouseInfo.button != sf::Mouse::Button::Left) {
		return;
	}

	// Reject mouse click if it's over an ImGui window.
	if (ImGui::IsMouseHoveringAnyWindow()) {
		return;
	}

	const auto windowSize = sf::Vector2i(GetContext().GetWindow().getSize());

	// Reject mouse click if it's outside the window.
	if (mouseInfo.x < 0 || mouseInfo.x >= windowSize.x) {
		return;
	}
	if (mouseInfo.y < 0 || mouseInfo.y >= windowSize.y) {
		return;
	}

	// Pass execution down to the tools layer.
	if (mInPreviewMode) {
		mTools.at(mCurrentToolIndex)->OnMouseClick3D(*this,
			mCamera.Get3D(),
			mouseInfo,
			(float)mFrameTex->getSize().x);
	}
	else {
		mTools.at(mCurrentToolIndex)->OnMouseClick(*this, mCamera.Get2D(), mouseInfo);
	}
}

void WorldEditor::OnMouseMove(const sf::Event::MouseMoveEvent & mouseInfo)
{
	// Reject mouse move if it's over an ImGui window.
	if (ImGui::IsMouseHoveringAnyWindow()) {
		return;
	}

	const auto windowSize = sf::Vector2i(GetContext().GetWindow().getSize());

	// Reject mouse move if it's outside the window.
	if (mouseInfo.x < 0 || mouseInfo.x >= windowSize.x) {
		return;
	}
	if (mouseInfo.y < 0 || mouseInfo.y >= windowSize.y) {
		return;
	}

	if (!mInPreviewMode) {
		// Pass execution down to the tools layer.
		mTools.at(mCurrentToolIndex)->OnMouseMove(*this, mCamera.Get2D(), mouseInfo);
	}
}

void WorldEditor::CameraPair::SetPosition(const b2Vec2& pos)
{
	mCamera2D.SetPosition(pos);
	mCamera3D.SetPosition(pos);
}

void WorldEditor::CameraPair::SetRotation(const float angle)
{
	mCamera2D.SetRotation(angle);
	mCamera3D.SetRotation(angle);
}

}