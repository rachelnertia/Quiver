#pragma once

#include <optional>
#include <string>
#include <vector>

#include <Box2D/Common/b2Math.h>
#include <SFML/Window/Event.hpp>
#include <SFML/System/Vector2.hpp>

namespace sf {
class RenderTarget;
}
class b2Body;

namespace qvr {

class Camera2D;
class Camera3D;
class WorldEditor;

class EditorTool {
public:
	EditorTool() {}

	virtual ~EditorTool() {}

	// Inheritors must provide a name.
	inline virtual const char* GetName() const = 0;
	// Description is optional.
	inline virtual const char* GetDescription() const { return nullptr; }

	// TODO: Raise the level of abstraction above 'mouse clicks'.
	// TODO: Add a 'step backwards' method that multi-step Tools can use.

	virtual void OnMouseClick(WorldEditor& editor, const Camera2D& camera, const sf::Event::MouseButtonEvent & mouseInfo) {};
	virtual void OnMouseMove(WorldEditor& editor, const Camera2D& camera, const sf::Event::MouseMoveEvent & mouseInfo) {};

	virtual void OnMouseClick3D(WorldEditor& editor,
		const Camera3D& camera,
		const sf::Event::MouseButtonEvent& mouseInfo,
		const float screenWidth) {};

	virtual void OnCancel(WorldEditor& editor, const Camera2D& camera) {};

	virtual void Draw(sf::RenderTarget& target, const Camera2D& camera) {};

	virtual void DoGui(WorldEditor& editor) {};

};

class NoTool : public EditorTool {
public:
	const char* GetName() const override { return "None"; }
};

class SelectTool : public EditorTool {
public:
	const char* GetName() const override { return "Select"; }
	const char* GetDescription() const override { return "Set the Editor's currently selected Entity"; }

	void OnMouseClick(WorldEditor& editor, const Camera2D& camera, const sf::Event::MouseButtonEvent & mouseInfo) override;

	void OnMouseClick3D(WorldEditor& editor,
		const Camera3D& camera,
		const sf::Event::MouseButtonEvent& mouseInfo,
		const float screenWidth) override;

};

class CopySelectedEntityTool : public EditorTool {
public:
	const char* GetName() const override { return "Copy Selected Entity"; }
	const char* GetDescription() const override { return "Make a copy of the currently-selected Entity where you click"; }

	void OnMouseClick(WorldEditor& editor, const Camera2D& camera, const sf::Event::MouseButtonEvent & mouseInfo) override;
};

class MoveTool : public EditorTool {
public:
	const char* GetName() const override { return "Move"; }

	void OnMouseClick(WorldEditor& editor, const Camera2D& camera, const sf::Event::MouseButtonEvent & mouseInfo) override;
	void OnMouseMove(WorldEditor& editor, const Camera2D& camera, const sf::Event::MouseMoveEvent & mouseInfo) override;

	void OnCancel(WorldEditor& editor, const Camera2D& camera) override;

private:
	b2Body* mBodyBeingMoved = nullptr;
	b2Vec2 mOriginalPos;
};

class RotateTool : public EditorTool {
public:
	const char* GetName() const override { return "Rotate"; }

	void OnMouseClick(WorldEditor& editor, const Camera2D& camera, const sf::Event::MouseButtonEvent & mouseInfo) override;
	void OnMouseMove(WorldEditor& editor, const Camera2D& camera, const sf::Event::MouseMoveEvent & mouseInfo) override;

	void OnCancel(WorldEditor& editor, const Camera2D& camera) override;

private:
	b2Body* mBody = nullptr;
	b2Vec2 mOriginalPos;
	float mOriginalAngle = 0.0f;
};

class CreateCircleTool : public EditorTool {
public:
	const char* GetName() const override { return "Create Circle"; }

	void OnMouseClick(WorldEditor& editor, const Camera2D& camera, const sf::Event::MouseButtonEvent & mouseInfo) override;
	void OnMouseMove(WorldEditor& editor, const Camera2D& camera, const sf::Event::MouseMoveEvent & mouseInfo) override;

	void OnCancel(WorldEditor& editor, const Camera2D& camera) override;

	void Draw(sf::RenderTarget& target, const Camera2D& camera) override;

private:
	bool mDragInProgress = false;
	b2Vec2 mDragStartPos;
	b2Vec2 mDragCurrentPos;
};

class CreateBoxTool : public EditorTool {
public:
	const char* GetName() const override { return "Create Box"; }

	void OnMouseClick(WorldEditor& editor, const Camera2D& camera, const sf::Event::MouseButtonEvent & mouseInfo) override;
	void OnMouseMove(WorldEditor& editor, const Camera2D& camera, const sf::Event::MouseMoveEvent & mouseInfo) override;

	void OnCancel(WorldEditor& editor, const Camera2D& camera) override;

	void Draw(sf::RenderTarget& target, const Camera2D& camera) override;

private:
	bool mDragInProgress = false;
	b2Vec2 mDragStartPos;
	b2Vec2 mDragCurrentPos;
};

class CreatePolygonTool : public EditorTool {
public:
	const char* GetName() const override { return "Create Polygon"; }
	const char* GetDescription() const override { return "Create a new Entity by defining a polygon shape"; }

	void OnMouseClick(WorldEditor& editor, const Camera2D& camera, const sf::Event::MouseButtonEvent & mouseInfo) override;
	void OnMouseMove(WorldEditor& editor, const Camera2D& camera, const sf::Event::MouseMoveEvent & mouseInfo) override;

	void DoGui(WorldEditor& editor) override;

	void OnCancel(WorldEditor& editor, const Camera2D& camera) override;

	void Draw(sf::RenderTarget& target, const Camera2D& camera) override;

private:
	b2Vec2 mLastMousePos;

	std::vector<b2Vec2> mVertices;

	std::optional<int> mGrabbedVertex;
};

class CreateInstanceOfPrefabTool : public EditorTool {
public:
	const char* GetName() const override { return "Create Instance of Prefab"; }

	void OnMouseClick(WorldEditor& editor, const Camera2D& camera, const sf::Event::MouseButtonEvent & mouseInfo) override;

	void DoGui(WorldEditor& editor);

private:
	std::string mCurrentPrefabName;

	int mCurrentPrefabIndex = -1;
};

}