#include "World.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>

#include <Box2D/Collision/Shapes/b2PolygonShape.h>
#include <Box2D/Common/b2Math.h>
#include <Box2D/Dynamics/b2Fixture.h>
#include <Box2D/Dynamics/b2World.h>
#include <Box2D/Dynamics/b2WorldCallbacks.h>
#include <Box2D/Dynamics/Contacts/b2Contact.h>
#include <ImGui/imgui.h>
#include <json.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <spdlog/spdlog.h>

// TODO: I don't like that we're including ApplicationState.h here.
//       I'd like to figure out a way to not have to do so.

#include "Quiver/Application/ApplicationState.h"
#include "Quiver/Audio/AudioLibrary.h"
#include "Quiver/Entity/Entity.h"
#include "Quiver/Entity/AudioComponent/AudioComponent.h"
#include "Quiver/Entity/CustomComponent/CustomComponent.h"
#include "Quiver/Entity/PhysicsComponent/PhysicsComponent.h"
#include "Quiver/Entity/PhysicsComponent/PhysicsComponentDef.h"
#include "Quiver/Entity/RenderComponent/RenderComponent.h"
#include "Quiver/Graphics/b2DrawSFML.h"
#include "Quiver/Graphics/Camera2D.h"
#include "Quiver/Graphics/Camera3D.h"
#include "Quiver/Graphics/ColourUtils.h"
#include "Quiver/Graphics/TextureLibrary.h"
#include "Quiver/Graphics/WorldRaycastRenderer.h"
#include "Quiver/Graphics/WorldUiRenderer.h"
#include "Quiver/Input/RawInput.h"
#include "Quiver/Misc/FindByAddress.h"
#include "Quiver/Misc/ImGuiHelpers.h"
#include "Quiver/Misc/JsonHelpers.h"
#include "Quiver/Misc/Profiler.h"
#include "Quiver/Physics/ContactFilter.h"
#include "Quiver/Physics/ContactListener.h"
#include "Quiver/World/WorldContext.h"

namespace qvr {

bool SaveWorld(const World & world, const std::string filename) {
	auto log = spdlog::get("console");
	assert(log.get());

	nlohmann::json j;

	if (!world.ToJson(j)) {
		return false;
	}

	std::ofstream out(filename);
	if (!out.is_open()) {
		return false;
	}

	out << j.dump(4); // dump with 4-space indenting

	out.close();

	log->debug("Serialized the World in JSON format to {}", filename);

	return true;
}

std::unique_ptr<World> LoadWorld(
	const std::string filename,
	WorldContext& worldContext)
{
	auto log = spdlog::get("console");
	assert(log.get());

	const nlohmann::json j = JsonHelp::LoadJsonFromFile(filename);

	try
	{
		auto world = std::make_unique<World>(worldContext, j);

		log->debug("Loaded World from JSON file {}", filename);

		return world;
	}
	catch (std::exception e)
	{
		log->error("World deserialization failed! Exception: {}", e.what());
	}

	return nullptr;
}

World::World(
	WorldContext& context)
	: mContext(context)
	, mContactListener(std::make_unique<Physics::ContactListener>())
	, mContactFilter(std::make_unique<Physics::ContactFilter>())
	, mPhysicsWorld(std::make_unique<b2World>(b2Vec2_zero))
	, mAudioLibrary(std::make_unique<AudioLibrary>())
	, mTextureLibrary(std::make_unique<TextureLibrary>())
{
	mPhysicsWorld->SetContactListener(mContactListener.get());
	//mPhysicsWorld->SetContactFilter(mContactFilter.get());
}

World::~World() {}

static Profiler sStepProfiler(512);

void World::TakeStep(qvr::RawInputDevices& inputDevices)
{
	using namespace std::chrono;

	ProfilerScope ps(sStepProfiler);

	// Update physics world.
	{
		int velocity_iterations = 8;
		int position_iterations = 2;
		mPhysicsWorld->Step(GetTimestep().count(), velocity_iterations, position_iterations);
	}

	for (auto& physicsComponent : mPhysicsComponents) {
		physicsComponent.get().Update(GetTimestep());
	}

	//for (auto& renderComponent : mRenderComponents) {
	//
	//}

	mAnimators.Animate(duration_cast<Animation::TimeUnit>(GetTimestep()));

	// TODO: Calculate total time a better way - float accumulation is error-prone.
	mTotalTime += GetTimestep();

	UpdateAudioComponents();

	m_CustomComponentUpdater.Update(GetTimestep(), inputDevices);

	// Look for CustomComponents with their remove flags set.
	{
		auto removeFlaggers = m_CustomComponentUpdater.GetRemoveFlaggers();
		for (auto customComponent : removeFlaggers) {
			RemoveEntityImmediate(customComponent.get().GetEntity());
		}
	}

	mStepCount += 1;
}

void World::SetPaused(const bool paused)
{
	if (paused)
	{
		
	}
	else
	{

	}

	for (auto& audioComponent : mAudioComponents)
	{
		audioComponent.get().SetPaused(paused);
	}

	mPaused = paused;
}

void World::RenderDebug(sf::RenderTarget & target, const Camera2D & camera)
{
	b2DrawSFML debugDraw;
	debugDraw.mTarget = &target;
	debugDraw.SetFlags(b2Draw::e_shapeBit | b2Draw::e_centerOfMassBit);
	debugDraw.mCamera = camera;
	mPhysicsWorld->SetDebugDraw(&debugDraw);
	mPhysicsWorld->DrawDebugData();
}

void World::Render2D(sf::RenderTarget& target, const Camera2D& camera)
{
	// Draw ground.
	{
		sf::RectangleShape rect;
		float fraction = std::max(0.0f, -mDirectionalLight.GetDirection().z);
		sf::Color directionalLightColor = mDirectionalLight.GetColor();
		directionalLightColor.r = (sf::Uint8)((float)directionalLightColor.r * fraction);
		directionalLightColor.g = (sf::Uint8)((float)directionalLightColor.g * fraction);
		directionalLightColor.b = (sf::Uint8)((float)directionalLightColor.b * fraction);
		rect.setFillColor(this->groundColor * this->mAmbientLight.mColor + directionalLightColor);
		rect.setSize(sf::Vector2f(target.getSize()));
		target.draw(rect);
	}

	RenderDebug(target, camera);
}

Profiler sPreRenderProfiler(512);
Profiler sRenderProfiler(512);
Profiler sColumnsProfiler(512);

void DrawGradientRectVertical(
	sf::RenderTarget& target,
	const sf::Vector2i topLeft,
	const sf::Vector2i bottomRight,
	const sf::Color topColor,
	const sf::Color bottomColor,
	const int steps = 1)
{
	auto lerp = [](float v0, float v1, float t) -> float {
		return (1 - t) * v0 + t * v1;
	};

	auto smoothStep = [lerp](float a, float b, float t) -> float {
		return lerp(a, b, (t * t)*(3 - (2 * t)));
	};

	auto accelerate = [lerp](float a, float b, float t) -> float {
		return lerp(a, b, t * t);
	};

	auto lerpUint8 = [](sf::Uint8 a, sf::Uint8 b, float t) -> sf::Uint8 {
		return (sf::Uint8)((1 - t) * a + t * b);
	};

	auto lerpColor = [lerpUint8](sf::Color c0, sf::Color c1, float t) -> sf::Color {
		return sf::Color(
			lerpUint8(c0.r, c1.r, t),
			lerpUint8(c0.g, c1.g, t),
			lerpUint8(c0.b, c1.b, t),
			lerpUint8(c0.a, c1.a, t));
	};

	auto smoothStepColor = [lerpColor](sf::Color a, sf::Color b, float t) -> sf::Color {
		return lerpColor(a, b, (t * t)*(3 - (2 * t)));
	};

	for (int step = 0; step < steps; step++) {
		const float t = ((float)step) / steps;
		const float t2 = ((float)step + 1) / steps;

		const int subRectTop = (int)lerp((float)topLeft.y, (float)bottomRight.y, t);
		const int subRectBottom = (int)lerp((float)topLeft.y, (float)bottomRight.y, t2);

		const sf::Color subRectTopColor = lerpColor(topColor, bottomColor, t);
		const sf::Color subRectBottomColor = lerpColor(topColor, bottomColor, t2);

		sf::Vertex verts[4] =
		{
			sf::Vertex(
				sf::Vector2f((float)topLeft.x, (float)subRectTop),
				subRectTopColor),
			sf::Vertex(
				sf::Vector2f((float)topLeft.x, (float)subRectBottom),
				subRectBottomColor),
			sf::Vertex(
				sf::Vector2f((float)bottomRight.x, (float)subRectBottom),
				subRectBottomColor),
			sf::Vertex(
				sf::Vector2f((float)bottomRight.x, (float)subRectTop),
				subRectTopColor)
		};

		target.draw(verts, 4, sf::PrimitiveType::Quads);
	}
}

void World::Render3D(
	sf::RenderTarget & target, 
	const Camera3D & camera,
	WorldRaycastRenderer & raycastRenderer)
{
	{
		ProfilerScope ps(sPreRenderProfiler);

		UpdateDetachedRenderComponents(camera);
	}

	const sf::Vector2u targetSize = target.getSize();

	if (sColumnsProfiler.BufferSize() != static_cast<int>(targetSize.x)) {
		sColumnsProfiler.Resize(targetSize.x);
	}

	// Draw ground.
	{
		sf::RectangleShape rect;
		const int top = (targetSize.y / 2) + GetPitchOffsetInPixels(camera, targetSize.y);
		rect.setPosition(0.0f, (float)top);
		rect.setSize(sf::Vector2f((float)targetSize.x, (float)(targetSize.y - top)));
		float fraction = std::max(0.0f, -mDirectionalLight.GetDirection().z);
		sf::Color directionalLightColor = mDirectionalLight.GetColor();
		directionalLightColor.r = (sf::Uint8)((float)directionalLightColor.r * fraction);
		directionalLightColor.g = (sf::Uint8)((float)directionalLightColor.g * fraction);
		directionalLightColor.b = (sf::Uint8)((float)directionalLightColor.b * fraction);
		rect.setFillColor(groundColor * mAmbientLight.mColor + directionalLightColor);
		target.draw(rect);

		// Draw distance-shade on top of it.
		{
			rect.setPosition(0.0f, (float)top);

			auto horizontalMetresToPixels = [](
				const int targetHeight,
				const float distanceMetres,
				const float cameraHeightMetres) -> int
			{
				return (int)((targetHeight / distanceMetres) * cameraHeightMetres);
			};

			const int maxIntensityPoint = horizontalMetresToPixels(
				targetSize.y,
				mFog.GetMaxDistance(),
				camera.GetHeight());

			rect.setSize(sf::Vector2f((float)targetSize.x, (float)maxIntensityPoint));

			const sf::Color maxIntensityColor(
				mFog.GetColor().r,
				mFog.GetColor().g,
				mFog.GetColor().b,
				(sf::Uint8)(mFog.GetMaxIntensity() * 255));

			rect.setFillColor(maxIntensityColor);

			target.draw(rect);

			const int minIntensityPoint = horizontalMetresToPixels(
				targetSize.y,
				mFog.GetMinDistance(),
				camera.GetHeight());

			const sf::Color minIntensityColor = mFog.GetColor();

			DrawGradientRectVertical(
				target,
				sf::Vector2i(0, (top + maxIntensityPoint)),
				sf::Vector2i((int)targetSize.x, (top + minIntensityPoint)),
				maxIntensityColor,
				minIntensityColor,
				1);
		}
	}
	// Draw sky.
	{
		// Draw background
		{
			sf::RectangleShape rect;
			rect.setPosition(0.0f, 0.0f);
			const float height = (float)(targetSize.y / 2) + GetPitchOffsetInPixels(camera, targetSize.y);
			rect.setSize(sf::Vector2f((float)targetSize.x, height));
			rect.setFillColor(skyColor);
			target.draw(rect);
		}

		mSky.Render(target, camera);
	}

	{
		ProfilerScope ps(sRenderProfiler);

		raycastRenderer.Render(*this, camera, mRenderSettings, target);
	}

	// Render stuff that goes on top of the 3D image (effects, HUD, weapons...)
	camera.DrawOverlay(target);
}

bool World::RegisterUiRenderer(WorldUiRenderer& renderer)
{
	const auto it = FindByAddress(mUiRenderers, renderer);

	if (it != mUiRenderers.end())
	{
		return false; // Double-registry is an error.
	}

	mUiRenderers.push_back(renderer);

	return true;
}

bool World::UnregisterUiRenderer(WorldUiRenderer& renderer)
{
	const auto it = FindByAddress(mUiRenderers, renderer);

	if (it == mUiRenderers.end())
	{
		return false; 
	}

	mUiRenderers.erase(it);

	return true;
}

void World::RenderUI(sf::RenderTarget& target) {
	for (WorldUiRenderer& renderer : mUiRenderers) {
		renderer.Render(target);
	}
}

Entity* World::CreateEntity(const b2Shape & shape, const b2Vec2 & position, const float angle)
{
	auto newEntity = std::make_unique<Entity>(*this, PhysicsComponentDef(shape, position, angle));

	Entity* ret = newEntity.get();

	AddEntity(std::move(newEntity));

	return ret;
}

Entity* World::CreateEntity(const nlohmann::json & j, const b2Transform * transform)
{
	std::unique_ptr<Entity> newEntity = Entity::FromJson(*this, j);

	if (!newEntity) {
		return nullptr;
	}

	if (transform) {
		newEntity->GetPhysics()->GetBody().SetTransform(transform->p, transform->q.GetAngle());
	}

	Entity* ret = newEntity.get();

	AddEntity(std::move(newEntity));

	return ret;
}

bool World::RemoveEntityImmediate(const Entity & entity)
{
	const auto it = mEntities.find(entity.GetId());

	if (it == mEntities.end()) return false;

	mEntities.erase(it);

	return true;
}

bool World::AddEntity(std::unique_ptr<Entity> entity)
{
	assert(entity != nullptr);
	assert(&entity->GetWorld() == this);
	assert(mEntities.count(entity->GetId()) == 0);

	if (entity == nullptr) return false;
	if (&entity->GetWorld() != this) return false;
	if (mEntities.count(entity->GetId()) != 0) return false;

	mEntities[entity->GetId()] = std::move(entity);

	return true;
}

namespace {

const char* animationsFieldName = "Animations";

}

bool World::ToJson(nlohmann::json & j) const {
	using json = nlohmann::json;

	assert(this->mPhysicsWorld);

	auto log = spdlog::get("console");
	assert(log.get());

	ColourUtils::SerializeSFColorToJson(this->groundColor, j["GroundColour"]);
	ColourUtils::SerializeSFColorToJson(this->skyColor, j["SkyColour"]);

	if (!mDirectionalLight.ToJson(j["DirectionalLight"])) {
		log->error("Failed to serialize directional light.");
	}

	j["AmbientLight"] = qvr::ToJson(mAmbientLight);

	j["Fog"] = qvr::ToJson(mFog);

	j["RenderSettings"] = mRenderSettings.ToJson();

	{
		b2Vec2 gravity = this->mPhysicsWorld->GetGravity();
		j["Gravity"] = { gravity.x, gravity.y };
	}

	mSky.ToJson(j["Sky"]);

	j[animationsFieldName] = mAnimators;

	unsigned serializedEntityCount = 0;

	for (const auto& entity : mEntities)
	{
		json entityData = entity.second->ToJson();
		if (entityData.empty()) {
			log->error("Entity serialization failed.");
			continue;
		}

		j["Entities"][serializedEntityCount] = entityData;

		serializedEntityCount++;
	}

	log->info("Serialized {} Entities.", serializedEntityCount);

	if (!mEntityPrefabs.ToJson(j["Prefabs"])) {
		log->error("Could not serialize Prefabs");
	}

	return true;
}

// At the moment, this doesn't throw any exceptions, I don't think (nothing in it can fail, we just fall back to defaults).
// In the future it might, so remember to put try { ... } around calls to this constructor.
World::World(
	WorldContext& customComponentTypes,
	const nlohmann::json & j)
	: World(customComponentTypes)
{
	auto log = spdlog::get("console");
	assert(log.get());

	if (j.find("GroundColour") != j.end()) {
		ColourUtils::DeserializeSFColorFromJson(groundColor, j["GroundColour"]);
	}

	if (j.find("SkyColour") != j.end()) {
		ColourUtils::DeserializeSFColorFromJson(skyColor, j["SkyColour"]);
	}

	if (j.find("Gravity") != j.end()) {
		mPhysicsWorld->SetGravity(b2Vec2(j["Gravity"][0], j["Gravity"][1]));
	}

	if (j.find("Sky") != j.end()) {
		mSky.FromJson(j["Sky"]);
	}

	mAmbientLight = FromJson(j.value<nlohmann::json>("AmbientLight", {}));

	mFog = Fog::FromJson(j.value<nlohmann::json>("Fog", {}));

	mRenderSettings = RenderSettings(j.value<nlohmann::json>("RenderSettings", {}));

	if (!(j.find("DirectionalLight") != j.end() &&
		mDirectionalLight.FromJson(j["DirectionalLight"])))
	{
		log->error("Failed to deserialize directional light.");
	}

	mAnimators = JsonHelp::GetValue<nlohmann::json>(j, animationsFieldName, {});

	if (j.find("Prefabs") != j.end()) {
		if (!mEntityPrefabs.FromJson(j["Prefabs"])) {
			log->error("Failed to deserialize any Prefabs.");
		}
	}

	if (j.find("Entities") != j.end()) {
		if (!j["Entities"].is_array()) {
			log->error("Found Entities field, but it's not an array.");
		}
		else {
			for (auto & jsonEntity : j["Entities"]) {
				auto entity = Entity::FromJson(*this, jsonEntity);
				if (!entity) {
					log->error("Failed to deserialize an Entity.");
					continue;
				}
				AddEntity(std::move(entity));
			}
		}
	}

	log->info("Deserialized {} Entitites.", mEntities.size());
}

bool World::RegisterCamera(const Camera3D& camera)
{
	const auto it = FindByAddress(mCameras, camera);

	if (it != mCameras.end())
	{
		return false; // Double-registry is an error.
	}

	mCameras.push_back(const_cast<Camera3D&>(camera));

	return true;
}

bool World::UnregisterCamera(const Camera3D& camera)
{
	const auto it = FindByAddress(mCameras, camera);

	if (it != mCameras.end())
	{
		const auto removedIndex = std::distance(std::cbegin(mCameras), it);

		if (mMainCameraIndex == removedIndex)
		{
			mMainCameraIndex = -1;
		}
		else if (mMainCameraIndex > removedIndex)
		{
			mMainCameraIndex -= 1;
		}

		mCameras.erase(it);

		return true;
	}

	return false;
}

bool World::SetMainCamera(const Camera3D& camera)
{
	auto log = spdlog::get("console");
	assert(log);

	const auto it = FindByAddress(mCameras, camera);

	if (it != mCameras.end())
	{
		log->info("Changing main camera index from {} to {}",
			mMainCameraIndex,
			std::distance(std::cbegin(mCameras), it));

		mMainCameraIndex = std::distance(std::cbegin(mCameras), it);

		return true;
	}

	return false;
}

const Camera3D* World::GetMainCamera() const
{
	if (mCameras.empty() ||
		mMainCameraIndex < 0 ||
		mMainCameraIndex >= (int)mCameras.size())
	{
		return nullptr;
	}

	return &(mCameras.at(mMainCameraIndex).get());
}

bool World::RegisterDetachedRenderComponent(const RenderComponent& renderComponent)
{
	assert(renderComponent.IsDetached());

	auto log = spdlog::get("console");
	assert(log != nullptr);

	static const char* logCtx = "World::RegisterDetachedRenderComponent:";

	const auto it = FindByAddress(mDetachedRenderComponents, renderComponent);

	if (it != mDetachedRenderComponents.end())
	{
		log->warn(
			"{} Trying to register a RenderComponent (index: {}, address: {:x}) a second time.",
			logCtx,
			it - mDetachedRenderComponents.begin(),
			(uintptr_t)&renderComponent);

		return true;
	}

	mDetachedRenderComponents.push_back(const_cast<RenderComponent&>(renderComponent));

	log->debug(
		"{} Registered a RenderComponent (address: {:x}) with index {}. "
		"There are now {} registered FlatSprites.",
		logCtx,
		(uintptr_t)&renderComponent,
		mDetachedRenderComponents.size() - 1,
		mDetachedRenderComponents.size());

	return true;
}

bool World::UnregisterDetachedRenderComponent(const RenderComponent& renderComponent)
{
	assert(renderComponent.IsDetached());

	auto log = spdlog::get("console");
	assert(log != nullptr);

	static const char* logCtx = "World::UnregisterDetachedRenderComponent:";

	const auto it = FindByAddress(mDetachedRenderComponents, renderComponent);

	if (it != mDetachedRenderComponents.end()) {
		mDetachedRenderComponents.erase(it);
		return true;
	}

	log->warn(
		"{} The given RenderComponent (address: {:x}) is not registered.",
		logCtx,
		(uintptr_t)&renderComponent);

	return false;
}

void World::UpdateDetachedRenderComponents(const Camera3D& camera)
{
	for (auto renderComp : mDetachedRenderComponents) {
		renderComp.get().UpdateDetachedBodyPosition();
		renderComp.get().UpdateDetachedBodyRotation(camera.GetRotation());
	}
}

bool World::RegisterAudioComponent(const AudioComponent & audioComponent)
{
	const auto it = FindByAddress(mAudioComponents, audioComponent);

	if (it != mAudioComponents.end())
	{
		return true;
	}

	mAudioComponents.push_back(const_cast<AudioComponent&>(audioComponent));

	return true;
}

bool World::UnregisterAudioComponent(const AudioComponent & audioComponent)
{
	const auto it = FindByAddress(mAudioComponents, audioComponent);

	if (it != mAudioComponents.end())
	{
		mAudioComponents.erase(it);
		return true;
	}

	return false;
}

bool World::RegisterPhysicsComponent(PhysicsComponent& physicsComponent)
{
	const auto it = FindByAddress(mPhysicsComponents, physicsComponent);

	if (it != mPhysicsComponents.end())
	{
		return true;
	}

	mPhysicsComponents.push_back(physicsComponent);

	return true;
}

bool World::UnregisterPhysicsComponent(PhysicsComponent& physicsComponent)
{
	const auto it = FindByAddress(mPhysicsComponents, physicsComponent);

	if (it != mPhysicsComponents.end())
	{
		mPhysicsComponents.erase(it);
		return true;
	}

	return false;
}

// Provide a factory function to create an ApplicationState.
// It will hopefully be grabbed by whatever owns and is responsible for updating the World.

void World::SetNextApplicationState(World::ApplicationStateCreator factoryFunc)
{
	mNextApplicationStateFactory = std::move(factoryFunc);
}

auto World::GetNextApplicationState(ApplicationStateContext & context) 
	-> std::unique_ptr<ApplicationState>
{
	if (mNextApplicationStateFactory) {
		auto appState = mNextApplicationStateFactory(context);
		mNextApplicationStateFactory = nullptr;
		return appState;
	}

	return nullptr;
}

void World::UpdateAudioComponents()
{
	for (auto& audioComponent : mAudioComponents)
	{
		audioComponent.get().Update();
	}
}

bool World::RegisterCustomComponent(const CustomComponent & customComponent)
{
	return m_CustomComponentUpdater.Register(const_cast<CustomComponent&>(customComponent));
}

bool World::UnregisterCustomComponent(const CustomComponent & customComponent)
{
	return m_CustomComponentUpdater.Unregister(const_cast<CustomComponent&>(customComponent));
}

void World::GuiControls() {
	auto log = spdlog::get("console");
	assert(log);

	if (ImGui::CollapsingHeader("General")) {
		ImGui::AutoIndent indent;

		ColourUtils::ImGuiColourEditRGB("Ground Colour", groundColor);

		{
			b2Vec2 gravity = mPhysicsWorld->GetGravity();

			ImGui::DragFloat2("Gravity", &gravity.x, 0.1f, -1.0f, 1.0f);

			mPhysicsWorld->SetGravity(gravity);
		}
	}

	if (ImGui::CollapsingHeader("Ambient Light")) {
		ImGui::AutoIndent indent;
		ColourUtils::ImGuiColourEditRGB("Colour", mAmbientLight.mColor);
	}

	if (ImGui::CollapsingHeader("Directional Light")) {
		ImGui::AutoIndent indent;
		mDirectionalLight.GuiControls();
	}

	if (ImGui::CollapsingHeader("Fog")) {
		ImGui::AutoIndent indent;
		qvr::GuiControls(mFog);
	}

	if (ImGui::CollapsingHeader("Cameras"))
	{
		ImGui::AutoIndent indent;

		if (!mCameras.empty()) {

			auto itemGetter = [](void* data, int index, const char** itemText)
			{
				auto cameras = static_cast<std::vector<std::reference_wrapper<Camera3D>>*>(data);

				if (index < 0) return false;
				if (index >= (int)cameras->size()) return false;

				*itemText = "Camera";

				return true;
			};

			const int prevMainCameraIndex = mMainCameraIndex;
			int mainCameraIndex = prevMainCameraIndex;

			if (ImGui::ListBox("Camera List",
				&mainCameraIndex,
				itemGetter,
				(void*)&mCameras,
				(int)mCameras.size()))
			{
				SetMainCamera(mCameras.at(mainCameraIndex));
			}
		}
		else {
			ImGui::Text("There are no registered cameras.");
		}
	}

	if (ImGui::CollapsingHeader("Sky")) {
		ImGui::AutoIndent indent;

		ColourUtils::ImGuiColourEdit("BG Colour", skyColor);

		mSky.EditorImGuiControls();
	}

	if (ImGui::CollapsingHeader("Raycast Renderer")) {
		ImGui::AutoIndent indent;

		ImGui::SliderFloat("Ray Length", &mRenderSettings.m_RayLength, 1.0f, 100.0f);
	}
}

void World::GuiPerformanceInfo()
{
	if (ImGui::CollapsingHeader("Render3D"))
	{
		ImGui::AutoIndent indent;

		std::string s = fmt::format(
			"n: {}, avg: {}ms",
			sPreRenderProfiler.BufferSize(),
			sPreRenderProfiler.GetAverage().count());

		ImGui::PlotLines(
			"Pre-Render",
			[](void* data, int idx)->float
			{
				auto profiler = (Profiler*)data;

				Profiler::SampleUnit sample = profiler->GetSample(idx);

				return sample.count();
			},
			&sPreRenderProfiler,
			sPreRenderProfiler.BufferSize(),
			0,
			s.c_str(),
			FLT_MAX,
			FLT_MAX,
			ImVec2(0, 80));

		s = fmt::format(
			"n: {}, avg: {}ms",
			sRenderProfiler.BufferSize(),
			sRenderProfiler.GetAverage().count());

		ImGui::PlotLines(
			"Render3D",
			[](void* data, int idx)->float {
				auto profiler = (Profiler*)data;

				Profiler::SampleUnit sample = profiler->GetSample(idx);

				return sample.count();
			},
			&sRenderProfiler,
			sRenderProfiler.BufferSize(),
			0,
			s.c_str(),
			FLT_MAX,
			FLT_MAX,
			ImVec2(0, 80));
	}

	if (ImGui::CollapsingHeader("TakeStep"))
	{
		ImGui::AutoIndent indent;

		std::string s = fmt::format(
			"n: {}, avg: {}ms",
			sStepProfiler.BufferSize(),
			sStepProfiler.GetAverage().count());

		ImGui::PlotLines(
			"TakeStep",
			[](void* data, int idx)->float {
				auto profiler = (Profiler*)data;

				Profiler::SampleUnit sample = profiler->GetSample(idx);

				return sample.count();
			},
			&sStepProfiler,
			sStepProfiler.BufferSize(),
			0,
			s.c_str(),
			FLT_MAX,
			FLT_MAX,
			ImVec2(0, 80));
	}
}

}
