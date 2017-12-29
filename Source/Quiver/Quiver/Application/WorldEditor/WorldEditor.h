#pragma once

#include <vector>

#include <Box2D/Common/b2Math.h>
#include <SFML/Window/Event.hpp>
#include <SFML/System/Clock.hpp>

#include "Quiver/Animation/AnimationEditor.h"
#include "Quiver/Animation/AnimationLibraryGui.h"
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

class EditorTool;
class Entity;
class EntityEditor;
class TextureLibraryGui;
class World;

class WorldEditor : public ApplicationState {
public:
	WorldEditor(ApplicationStateContext& context);
	WorldEditor(ApplicationStateContext& context, std::unique_ptr<World> world);

	~WorldEditor();

	WorldEditor(const WorldEditor&) = delete;
	WorldEditor(const WorldEditor&&) = delete;

	WorldEditor& operator=(const WorldEditor&) = delete;
	WorldEditor& operator=(const WorldEditor&&) = delete;

	void ProcessFrame() override;

	inline World* GetWorld() { return mWorld.get(); }

	inline Entity* GetCurrentSelection() { return mCurrentSelection; }

	inline void SetCurrentSelection(Entity* entity) { mCurrentSelection = entity; }

private:
	void HandleInput(const float dt);
	void Render();

	// ImGui calls go here.
	void ProcessGUI();

	void OnMouseClick(const sf::Event::MouseButtonEvent& mouseInfo);
	void OnMouseMove(const sf::Event::MouseMoveEvent& mouseInfo);

	std::unique_ptr<World> mWorld;

	WorldRaycastRenderer mWorldRaycastRenderer;

	// TODO: Can probably do away with mCurrentSelection and just have the EntityEditor.
	Entity* mCurrentSelection = nullptr;
	std::unique_ptr<EntityEditor> mCurrentSelectionEditor;

	std::vector<std::unique_ptr<EditorTool>> mTools;

	// Index of the currently-active EditorTool in the mTools array.
	int mCurrentToolIndex = 0;

	bool mInPreviewMode = false;

	struct CameraPair {

		Camera2D& Get2D() { return mCamera2D; }
		Camera3D& Get3D() { return mCamera3D; }

		void SetPosition(const b2Vec2& pos);
		void SetRotation(const float angle);

		//void Sync();

		Camera2D mCamera2D;
		Camera3D mCamera3D;
	};

	CameraPair mCamera;

	sf::Clock frameClock;

	std::unique_ptr<sf::RenderTexture> mFrameTex;

	float mFrameTexResolutionModifier = 1.0f;

	AnimationEditor mAnimationEditor;

	AnimationLibraryEditorData mAnimationLibraryEditorData;

	std::string mWorldFilename;

	std::unique_ptr<TextureLibraryGui> mTextureLibraryGui;

	qvr::SfmlJoystickSet mJoysticks;
	qvr::SfmlKeyboard mKeyboard;
	qvr::SfmlMouse mMouse;
};

}