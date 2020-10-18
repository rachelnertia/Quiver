#include "EditorTools.h"

#include <iostream>

#include <Box2D/Common/b2Math.h>
#include <Box2D/Collision/b2Collision.h>
#include <Box2D/Collision/Shapes/b2CircleShape.h>
#include <Box2D/Collision/Shapes/b2PolygonShape.h>
#include <Box2D/Dynamics/b2World.h>
#include <Box2D/Dynamics/b2WorldCallbacks.h>
#include <Box2D/Dynamics/b2Fixture.h>
#include <ImGui/imgui.h>
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RectangleShape.hpp>

#include "Quiver/Application/WorldEditor/WorldEditor.h"
#include "Quiver/Entity/Entity.h"
#include "Quiver/Entity/PhysicsComponent/PhysicsComponent.h"
#include "Quiver/Entity/RenderComponent/RenderComponent.h"
#include "Quiver/Graphics/Camera2D.h"
#include "Quiver/Graphics/Camera3D.h"
#include "Quiver/Misc/ImGuiHelpers.h"
#include "Quiver/Misc/Logging.h"
#include "Quiver/World/World.h"

namespace {

b2Fixture* GetFixtureAtPoint(const b2World& world, const b2Vec2& point)
{
	class SelectClickCallback : public b2QueryCallback {
	public:
		std::vector<b2Fixture*> mFixtures;

		bool ReportFixture(b2Fixture* fixture) override
		{
			mFixtures.push_back(fixture);
			return true;
		}
	};

	// A very small AABB, centered on the mouse, scaled by the pixels-to-metres ratio.
	b2AABB aabb;
	// AABB is 2 pixels wide.
	float halfWidth = 1.0f / 32.0f;
	aabb.lowerBound = point - b2Vec2(halfWidth, halfWidth);
	aabb.upperBound = point + b2Vec2(halfWidth, halfWidth);

	SelectClickCallback callback;

	world.QueryAABB(&callback, aabb);

	for (b2Fixture* fixture : callback.mFixtures) {
		if (fixture->TestPoint(point)) {
			return fixture;
		}
	}

	return nullptr;
}

}

namespace qvr {

void SelectTool::OnMouseClick(WorldEditor & editor, const Camera2D& camera, const sf::Event::MouseButtonEvent & mouseInfo)
{
	auto log = GetConsoleLogger();

	b2Vec2 clickPos = camera.ScreenToWorld(b2Vec2((float)mouseInfo.x, (float)mouseInfo.y));

	log->info("LMB clicked at ({}, {}).", clickPos.x, clickPos.y);

	const b2Fixture* foundFixture = GetFixtureAtPoint(*editor.GetWorld()->GetPhysicsWorld(), clickPos);

	if (foundFixture) {
		void* bodyUserData = foundFixture->GetBody()->GetUserData();

		if (bodyUserData) {
			// Follow the fixture's b2Body to the Entity
			PhysicsComponent* physicsComponent = (PhysicsComponent*)bodyUserData;
			editor.SetCurrentSelection(&physicsComponent->GetEntity());

			log->info("Selected an Entity.");

			return;
		}
	}

	log->info("Found nothing.");
	
	if (editor.GetCurrentSelection()) {
		editor.SetCurrentSelection(nullptr);
		log->info("Deselected an Entity.");
	}
}

void SelectTool::OnMouseClick3D(WorldEditor & editor,
	const Camera3D & camera,
	const sf::Event::MouseButtonEvent & mouseInfo,
	const float screenWidth)
{
	// Just get the first fixture we hit.
	class Callback : public b2RayCastCallback {
	public:
		b2Fixture * closestFixture = nullptr;

		float32 ReportFixture(
			b2Fixture* fixture,
			const b2Vec2& point,
			const b2Vec2& normal,
			float32 fraction) override
		{
			// Filter out render-only fixtures
			if ((fixture->GetFilterData().categoryBits & 0xF000) != 0)
			{
				return -1;
			}

			closestFixture = fixture;

			return fraction;
		}
	};

	Callback callback;

	b2Vec2 rayEnd;
	{
		// The 'view plane' vector is the camera's right-vector, stretched/squashed a bit:
		const b2Vec2 viewPlane(
			camera.GetForwards().y * camera.GetViewPlaneWidthModifier() * -1.0f,
			camera.GetForwards().x * camera.GetViewPlaneWidthModifier());

		const float screenX = -1.0f + mouseInfo.x * (2.0f / screenWidth);

		b2Vec2 rayDir = camera.GetForwards() + (screenX * viewPlane);
		rayDir.Normalize();

		const float rayLength = 30.0f;
		rayEnd = camera.GetPosition() + (rayLength * rayDir);
	}

	editor.GetWorld()->GetPhysicsWorld()->RayCast(&callback, camera.GetPosition(), rayEnd);

	if (callback.closestFixture) {
		// Follow the fixture's PhysicsComponent to the Entity.
		auto physComp =
			(PhysicsComponent*)(callback.closestFixture->GetBody()->GetUserData());
		editor.SetCurrentSelection(&physComp->GetEntity());
	}
	else {
		editor.SetCurrentSelection(nullptr);
	}
}

void CopySelectedEntityTool::OnMouseClick(WorldEditor & editor, const Camera2D & camera, const sf::Event::MouseButtonEvent & mouseInfo)
{
	auto log = GetConsoleLogger();

	if (editor.GetCurrentSelection() == nullptr) {
		log->info("No Entity is selected.");
		return;
	}

	b2Vec2 clickPos = camera.ScreenToWorld(b2Vec2((float)mouseInfo.x, (float)mouseInfo.y));

	log->info("LMB clicked at ({}, {}).", clickPos.x, clickPos.y);
	log->info("Copying Entity {X}", (void*)editor.GetCurrentSelection());

	const nlohmann::json j = editor.GetCurrentSelection()->ToJson();

	std::unique_ptr<Entity> entity = Entity::FromJson(*editor.GetWorld(), j);
	if (!entity) {
		return;
	}

	const b2Transform t(clickPos, camera.mTransform.q);
	entity->GetPhysics()->GetBody().SetTransform(t.p, t.q.GetAngle());

	log->info("New Entity: {X}", (void*)entity.get());

	if (editor.GetWorld()->AddEntity(std::move(entity)))
	{
		log->debug("Added new Entity to the World.");
	}
	else
	{
		log->error("Failed to add Entity to World.");
	}
}

void CreateCircleTool::OnMouseClick(WorldEditor & editor, const Camera2D& camera, const sf::Event::MouseButtonEvent & mouseInfo)
{
	b2Vec2 clickPos = camera.ScreenToWorld(b2Vec2((float)mouseInfo.x, (float)mouseInfo.y));

	if (mDragInProgress) {
		// Circle completed. 
		mDragInProgress = false;
		// Create an Entity.
		b2CircleShape circleShape;
		circleShape.m_radius = (mDragCurrentPos - mDragStartPos).Length();
		auto entity = editor.GetWorld()->CreateEntity(circleShape, mDragStartPos, camera.GetRotation());
		// Give it a RenderComponent.
		entity->AddGraphics();
	}
	else {
		// Begin dragging.
		mDragInProgress = true;
		mDragStartPos = clickPos;
		mDragCurrentPos = clickPos;
	}
}

void CreateCircleTool::OnMouseMove(WorldEditor & editor, const Camera2D& camera, const sf::Event::MouseMoveEvent & mouseInfo)
{
	if (!mDragInProgress) {
		return;
	}

	b2Vec2 mouseWorldPos = camera.ScreenToWorld(b2Vec2((float)mouseInfo.x, (float)mouseInfo.y));

	mDragCurrentPos = mouseWorldPos;
}

void CreateCircleTool::OnCancel(WorldEditor & editor, const Camera2D& camera)
{
	mDragInProgress = false;
}

void CreateCircleTool::Draw(sf::RenderTarget& target, const Camera2D& camera)
{
	if (!mDragInProgress) {
		return;
	}

	sf::CircleShape circle;
	float radius = camera.MetresToPixels((mDragCurrentPos - mDragStartPos).Length());
	circle.setRadius(radius);
	circle.setOrigin(sf::Vector2f(radius, radius));
	b2Vec2 circleCentre = camera.WorldToCamera(mDragStartPos);
	circle.setPosition(sf::Vector2f(circleCentre.x, circleCentre.y));

	target.draw(circle);
}

void CreateBoxTool::OnMouseClick(WorldEditor & editor, const Camera2D& camera, const sf::Event::MouseButtonEvent & mouseInfo)
{
	b2Vec2 mouseWorldPos = camera.ScreenToWorld(b2Vec2((float)mouseInfo.x, (float)mouseInfo.y));

	if (mDragInProgress) {
		// Box completed. 
		mDragInProgress = false;
		// Create a box-shaped Entity.
		b2PolygonShape polygonShape;
		b2Vec2 dist = mDragCurrentPos - mDragStartPos;
		dist = b2MulT(camera.mTransform.q, dist);
		polygonShape.SetAsBox(abs(dist.x) / 2, abs(dist.y) / 2);
		dist = mDragCurrentPos - mDragStartPos;
		auto entity =
			editor.GetWorld()->CreateEntity(
				polygonShape,
				mDragStartPos + (0.5f * dist),
				camera.GetRotation());
		// Give it a RenderComponent.
		entity->AddGraphics();
	}
	else {
		// Begin dragging.
		mDragInProgress = true;
		mDragStartPos = mouseWorldPos;
		mDragCurrentPos = mouseWorldPos;
	}
}

void CreateBoxTool::OnMouseMove(WorldEditor & editor, const Camera2D& camera, const sf::Event::MouseMoveEvent & mouseInfo)
{
	if (!mDragInProgress) {
		return;
	}

	b2Vec2 mouseWorldPos = camera.ScreenToWorld(b2Vec2((float)mouseInfo.x, (float)mouseInfo.y));

	mDragCurrentPos = mouseWorldPos;
}

void CreateBoxTool::OnCancel(WorldEditor & editor, const Camera2D& camera)
{
	mDragInProgress = false;
}

void CreateBoxTool::Draw(sf::RenderTarget& target, const Camera2D& camera)
{
	if (!mDragInProgress) {
		return;
	}

	sf::RectangleShape rectangle;
	b2Vec2 dist = mDragCurrentPos - mDragStartPos;
	dist = b2MulT(camera.mTransform.q, dist);
	dist.x = camera.MetresToPixels(dist.x);
	dist.y = camera.MetresToPixels(dist.y);
	rectangle.setSize(sf::Vector2f(dist.x, dist.y));
	b2Vec2 rectTopLeft = camera.WorldToCamera(mDragStartPos);
	rectangle.setPosition(sf::Vector2f(rectTopLeft.x, rectTopLeft.y));

	target.draw(rectangle);
}

void MoveTool::OnMouseClick(WorldEditor & editor, const Camera2D& camera, const sf::Event::MouseButtonEvent & mouseInfo)
{
	b2Vec2 mouseWorldPos = camera.ScreenToWorld(b2Vec2((float)mouseInfo.x, (float)mouseInfo.y));

	// If we're already moving a body, finalise its position.
	if (mBodyBeingMoved) {
		mBodyBeingMoved = nullptr;
		return;
	}

	// Pick the fixture beneath the mouse cursor, if it exists. If it doesn't, bail out.
	b2Fixture* foundFixture = GetFixtureAtPoint(*editor.GetWorld()->GetPhysicsWorld(), mouseWorldPos);

	if (!foundFixture) {
		return;
	}

	// Get the body that owns the fixture and store it.
	mBodyBeingMoved = foundFixture->GetBody();

	// Store its position before moving it in case we cancel the move.
	mOriginalPos = mBodyBeingMoved->GetPosition();
}

void MoveTool::OnMouseMove(WorldEditor & editor, const Camera2D& camera, const sf::Event::MouseMoveEvent & mouseInfo)
{
	if (!mBodyBeingMoved) {
		return;
	}

	b2Vec2 pos = camera.ScreenToWorld(b2Vec2((float)mouseInfo.x, (float)mouseInfo.y));

	mBodyBeingMoved->SetTransform(pos, mBodyBeingMoved->GetAngle());
}

void MoveTool::OnCancel(WorldEditor & editor, const Camera2D& camera)
{
	if (!mBodyBeingMoved) {
		return;
	}

	// Move the body back to its original position.
	mBodyBeingMoved->SetTransform(mOriginalPos, mBodyBeingMoved->GetAngle());

	mBodyBeingMoved = nullptr;
}

void RotateTool::OnMouseClick(WorldEditor & editor, const Camera2D& camera, const sf::Event::MouseButtonEvent & mouseInfo)
{
	b2Vec2 clickPos = camera.ScreenToWorld(b2Vec2((float)mouseInfo.x, (float)mouseInfo.y));

	// If we're already moving a body, finalise its position.
	if (mBody) {
		mBody = nullptr;
		return;
	}

	// Pick the fixture beneath the mouse cursor, if it exists. If it doesn't, bail out.
	b2Fixture* foundFixture = GetFixtureAtPoint(*editor.GetWorld()->GetPhysicsWorld(), clickPos);

	if (!foundFixture) {
		return;
	}

	// Get the body that owns the fixture and store it.
	mBody = foundFixture->GetBody();

	mOriginalPos = mBody->GetPosition();
	mOriginalAngle = mBody->GetAngle();
}

void RotateTool::OnMouseMove(WorldEditor & editor, const Camera2D& camera, const sf::Event::MouseMoveEvent & mouseInfo)
{
	if (!mBody) {
		return;
	}

	b2Vec2 pos = camera.ScreenToWorld(b2Vec2((float)mouseInfo.x, (float)mouseInfo.y));

	float angle = atan2f(pos.y - mOriginalPos.y, pos.x - mOriginalPos.x);

	mBody->SetTransform(mBody->GetPosition(), angle);
}

void RotateTool::OnCancel(WorldEditor & editor, const Camera2D& camera)
{
	if (!mBody) {
		return;
	}

	mBody->SetTransform(mBody->GetPosition(), mOriginalAngle);
}

void CreateInstanceOfPrefabTool::DoGui(WorldEditor& editor)
{
	const auto& prefabNames = editor.GetWorld()->mEntityPrefabs.GetPrefabNames();

	if (ImGui::ListBox("Available Prefabs", &mCurrentPrefabIndex, &prefabNames[0], prefabNames.size()))
	{
		mCurrentPrefabName = prefabNames[mCurrentPrefabIndex];
	}
}

void CreateInstanceOfPrefabTool::OnMouseClick(WorldEditor & editor, const Camera2D& camera, const sf::Event::MouseButtonEvent & mouseInfo)
{
	auto log = GetConsoleLogger();

	constexpr const char* logContext = "CreateInstanceOfPrefabTool::OnMouseClick:";

	if (mCurrentPrefabIndex < 0) {
		log->info("{} No Prefab is selected.", logContext);
	}

	const auto prefabs = editor.GetWorld()->mEntityPrefabs;

	const auto prefab = prefabs.GetPrefab(mCurrentPrefabName);

	if (!prefab) {
		log->error("{} Selected Prefab is not real, or something.");
		mCurrentPrefabIndex = -1;
		mCurrentPrefabName.clear();
		return;
	}

	// Create Entity.
	{
		const b2Vec2 clickPos = camera.ScreenToWorld(b2Vec2((float)mouseInfo.x, (float)mouseInfo.y));

		const b2Transform transform(clickPos, camera.mTransform.q);

		Entity* entity = editor.GetWorld()->CreateEntity(*prefab, &transform);

		if (entity == nullptr) {
			log->error("{} Failed to instantiate Prefab {}.", logContext, mCurrentPrefabName);
			return;
		}

		entity->SetPrefab(mCurrentPrefabName);

		log->info("{} Successfully instantiated Prefab {}.", logContext, mCurrentPrefabName);
	}
}

std::optional<int> GetIndexOfClosestPoint(
	const std::vector<b2Vec2>& points,
	const b2Vec2 testPoint,
	const float radius)
{
	for (int i = 0; i < (int)points.size(); ++i) {
		if ((testPoint - points[i]).LengthSquared() <= (radius * radius)) {
			return i;
		}
	}

	return {};
}

static const int MaxNumVertices = 8;
static const int MinNumVertices = 3;

// Determines whether the given point is a valid addition to the convex hull defined by 
// points.
bool IsValidConvexShape(const std::vector<b2Vec2>& points)
{
	assert(points.size() <= MaxNumVertices);
	assert(points.size() >= MinNumVertices);
	b2PolygonShape shape;
	for (int i = 0; i < (int)points.size(); i++) {
		shape.m_vertices[i] = points[i];
	}
	shape.m_count = points.size();
	return shape.Validate();
}

static const float RadiusMetres = 0.1f;

b2Vec2 GetVecFromMouseEvent(const sf::Event::MouseButtonEvent & event)
{
	return b2Vec2((float)event.x, (float)event.y);
}

b2Vec2 GetVecFromMouseEvent(const sf::Event::MouseMoveEvent & event)
{
	return b2Vec2((float)event.x, (float)event.y);
}

void CreatePolygonTool::OnMouseClick(WorldEditor & editor, const Camera2D & camera, const sf::Event::MouseButtonEvent & mouseInfo)
{
	if (mouseInfo.button == sf::Mouse::Button::Left) {
		const b2Vec2 worldPos = camera.ScreenToWorld(GetVecFromMouseEvent(mouseInfo));
		if (mGrabbedVertex) {
			// Change the position of an existing vertex.
			const b2Vec2 originalValue = mVertices[*mGrabbedVertex];
			mVertices[*mGrabbedVertex] = worldPos;
			if ((mVertices.size() < MinNumVertices) || IsValidConvexShape(mVertices)) {
				// Success.
				mGrabbedVertex.reset();
			}
			else {
				// Undo.
				mVertices[*mGrabbedVertex] = originalValue;
			}
		}
		else {
			// Place a new vertex or select an existing one.
			{
				// TODO: This probably ought to scale based on how zoomed out the camera is.
				const float radius = RadiusMetres; 
				mGrabbedVertex = GetIndexOfClosestPoint(mVertices, worldPos, radius);
			}
			if (!mGrabbedVertex) {
				if (mVertices.size() < MaxNumVertices) {
					mVertices.push_back(worldPos);
					const int minNumVertices = 3;
					if (mVertices.size() >= MinNumVertices) {
						if (!IsValidConvexShape(mVertices)) {
							// Undo.
							mVertices.pop_back();
						}
					}
				}
			}
		}
	}
	else if (mouseInfo.button == sf::Mouse::Button::Right) {
		// Go back a step.
		if (mGrabbedVertex) {
			mGrabbedVertex.reset();
		}
		else {
			mVertices.pop_back();
		}
	}
}

void CreatePolygonTool::OnMouseMove(WorldEditor & editor, const Camera2D & camera, const sf::Event::MouseMoveEvent & mouseInfo)
{
	mLastMousePos = camera.ScreenToWorld(GetVecFromMouseEvent(mouseInfo));
}

void CreatePolygonTool::DoGui(WorldEditor & editor)
{
	if ((mVertices.size() >= MinNumVertices) && IsValidConvexShape(mVertices)) {
		if (ImGui::Button("Finish")) {
			b2PolygonShape polygonShape;
			polygonShape.Set(&mVertices[0], mVertices.size());
			// Translate polygonShape back to the origin.
			const b2Vec2 centroid = polygonShape.m_centroid;
			polygonShape.m_centroid.SetZero();
			for (b2Vec2& vertex : polygonShape.m_vertices) {
				vertex -= centroid;
			}
			auto entity =
				editor.GetWorld()->CreateEntity(
					polygonShape,
					centroid,
					0.0f);
			// Give it a RenderComponent.
			entity->AddGraphics();

			mVertices.clear();
			mGrabbedVertex.reset();
		}
	}
}

void CreatePolygonTool::OnCancel(WorldEditor & editor, const Camera2D & camera)
{
	mVertices.clear();
	mGrabbedVertex.reset();
}

sf::Vector2f b2VecToSfVec(const b2Vec2& vec) {
	return sf::Vector2f{ vec.x, vec.y };
}

void CreatePolygonTool::Draw(sf::RenderTarget & target, const Camera2D & camera)
{
	using namespace std;

	array<sf::Vertex, MaxNumVertices> verts;

	const int n = std::min((int)mVertices.size(), MaxNumVertices);
	for (int i = 0; i < n; i++) {
		verts[i].position = b2VecToSfVec(camera.WorldToCamera(mVertices[i]));
	}

	target.draw(&verts[0], mVertices.size(), sf::PrimitiveType::LinesStrip);

	for (int i = 0; i < n; i++) {
		sf::RectangleShape shape;
		shape.setPosition(verts[i].position);
		shape.setSize(sf::Vector2f(2.0f, 2.0f));
		target.draw(shape);
	}

	if (mGrabbedVertex) {
		const bool isValid = [this]()
		{
			if (mVertices.size() < (MinNumVertices - 1)) return true;

			const b2Vec2 originalValue = mVertices[*mGrabbedVertex];
			mVertices[*mGrabbedVertex] = mLastMousePos;
			const bool valid = IsValidConvexShape(mVertices);
			mVertices[*mGrabbedVertex] = originalValue;
			return valid;
		}();

		const sf::Color color = isValid ? sf::Color::Green : sf::Color::Red;

		{
			const int baseIndex = (*mGrabbedVertex > 0) ?
				(*mGrabbedVertex - 1) :
				((mVertices.size() >= 2) ? 1 : -1);

			array<sf::Vertex, 2> line;
			line[0].color = color;
			line[1].color = color;

			if (baseIndex >= 0) {
				line[0].position = verts[baseIndex].position;
				line[1].position = b2VecToSfVec(camera.WorldToCamera(mLastMousePos));
				target.draw(&line[0], 2, sf::PrimitiveType::Lines);
			}

			if ((*mGrabbedVertex + 1) < (int)mVertices.size()) {
				line[0].position = b2VecToSfVec(camera.WorldToCamera(mLastMousePos));
				line[1].position = verts[*mGrabbedVertex + 1].position;
				target.draw(&line[0], 2, sf::PrimitiveType::Lines);
			}
		}

		{
			sf::RectangleShape shape;
			shape.setPosition(b2VecToSfVec(camera.WorldToCamera(mLastMousePos)));
			shape.setSize(sf::Vector2f(4.0f, 4.0f));
			shape.setFillColor(color);
			target.draw(shape);
		}
	}
	else {
		const auto index = GetIndexOfClosestPoint(mVertices, mLastMousePos, RadiusMetres);
		if (index) {
			sf::CircleShape circle;
			circle.setFillColor(sf::Color::Transparent);
			circle.setOutlineColor(sf::Color::Green);
			circle.setOutlineThickness(2.0f);
			circle.setPosition(b2VecToSfVec(camera.WorldToCamera(mVertices[*index])));
			const float radiusPixels = camera.MetresToPixels(RadiusMetres);
			circle.setRadius(radiusPixels);
			circle.setOrigin(radiusPixels, radiusPixels);
			target.draw(circle);
		}
		else if (mVertices.size() < MaxNumVertices) {
			sf::RectangleShape shape;
			shape.setPosition(b2VecToSfVec(camera.WorldToCamera(mLastMousePos)));
			shape.setSize(sf::Vector2f(4.0f, 4.0f));

			const bool isValid = [this]()
			{
				if (mVertices.size() < (MinNumVertices - 1)) return true;

				mVertices.push_back(mLastMousePos);
				const bool valid = IsValidConvexShape(mVertices);
				mVertices.pop_back();
				return valid;
			}();

			shape.setFillColor(isValid ? sf::Color::Green : sf::Color::Red);
			target.draw(shape);
		}
	}
}

}