#pragma once

#include <chrono>
#include <vector>

#include <Box2D/Common/b2Math.h>
#include <function2.hpp>
#include <json.hpp>
#include <optional.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Texture.hpp>

#include "Quiver/Animation/Animators.h"
#include "Quiver/Entity/CustomComponent/CustomComponentUpdater.h"
#include "Quiver/Entity/EntityId.h"
#include "Quiver/Entity/EntityPrefab.h"
#include "Quiver/Graphics/Fog.h"
#include "Quiver/Graphics/Light.h"
#include "Quiver/Graphics/RenderSettings.h"
#include "Quiver/Graphics/Sky.h"
#include "Quiver/World/WorldContext.h"

struct b2Transform;
struct b2Vec2;
class b2Body;
class b2ContactListener;
class b2Shape;
class b2World;

namespace sf {
class RenderTarget;
}

namespace qvr {

class ApplicationState;
class ApplicationStateContext;
class AudioComponent;
class AudioLibrary;
class Camera2D;
class Camera3D;
class CustomComponent;
class CustomComponentTypeLibrary;
class Entity;
class EntityPrefab;
class RawInputDevices;
class RenderComponent;
class TextureLibrary;
class World;
class WorldContext;
class WorldRaycastRenderer;
class WorldUiRenderer;

bool SaveWorld(
	const World & world, 
	const std::string filename);

std::unique_ptr<World> LoadWorld(
	const std::string filename, 
	WorldContext& context);

class World {
public:
	World(WorldContext& context);

	World(
		WorldContext& context,
		const nlohmann::json& j);

	~World();

	World(const World&) = delete;
	World(const World&&) = delete;

	World& operator=(const World&) = delete;
	World& operator=(const World&&) = delete;

	void TakeStep(qvr::RawInputDevices& inputDevices);

	// While paused is true, TakeStep will do nothing.
	void SetPaused(const bool paused);

	void RenderDebug(
		sf::RenderTarget& target, 
		const Camera2D& camera);
	
	void Render3D(
		sf::RenderTarget& target, 
		const Camera3D& camera,
		WorldRaycastRenderer& raycastRenderer);

	void RenderUI(sf::RenderTarget& target);

	Entity* CreateEntity(const b2Shape & shape, const b2Vec2 & position, const float angle = 0.0f);
	Entity* CreateEntity(const nlohmann::json & json, const b2Transform* transform = nullptr);

	bool AddEntity(std::unique_ptr<Entity> entity);

	Entity* GetEntity(const EntityId id) {
		if (id == EntityId(0)) return nullptr;
		
		const auto it = mEntities.find(id);

		if (it == mEntities.end()) return nullptr;

		return it->second.get();
	}

	bool RemoveEntityImmediate(const Entity& entity);

	void GuiControls();
	void GuiPerformanceInfo();

	bool ToJson(nlohmann::json & j) const;

	bool SetMainCamera(const Camera3D& camera);

	const Camera3D* GetMainCamera() const;

	bool RegisterCamera(const Camera3D& camera);
	bool UnregisterCamera(const Camera3D& camera);

	bool RegisterDetachedRenderComponent(const RenderComponent& renderComponent);
	bool UnregisterDetachedRenderComponent(const RenderComponent& renderComponent);

	void UpdateDetachedRenderComponents(const Camera3D& camera);

	bool RegisterAudioComponent(const AudioComponent& audioComponent);
	bool UnregisterAudioComponent(const AudioComponent& audioComponent);

	bool RegisterCustomComponent(const CustomComponent& customComponent);
	bool UnregisterCustomComponent(const CustomComponent& customComponent);

	bool RegisterUiRenderer(WorldUiRenderer& renderer);
	bool UnregisterUiRenderer(WorldUiRenderer& renderer);

	inline std::chrono::duration<float> GetTimestep() const { return mTimestep; }

	inline const DirectionalLight& GetDirectionalLight() const { return mDirectionalLight; }

	inline const b2World* GetPhysicsWorld() const { return mPhysicsWorld.get(); }

	AnimatorCollection& GetAnimators() { return mAnimators; }
	AudioLibrary&    GetAudioLibrary() { return *mAudioLibrary.get(); }
	TextureLibrary&  GetTextureLibrary() { return *mTextureLibrary.get(); }

	EntityId GetNextEntityId() { 
		EntityId id = mNextEntityId; 
		mNextEntityId = EntityId(mNextEntityId.get() + 1);
		return id;
	}

	EntityPrefabContainer mEntityPrefabs;

	sf::Color groundColor = sf::Color::Yellow;
	sf::Color skyColor = sf::Color::Transparent;

	// TODO: Some kind of World clock thing with time_point
	using TimePoint = std::chrono::duration<float>;

	TimePoint GetTime() const { return mTotalTime; }

	const AmbientLight& GetAmbientLight() const { return mAmbientLight; }
	const Fog&          GetFog()          const { return mFog; }

	CustomComponentTypeLibrary& GetCustomComponentTypes() const {
		return mContext.GetCustomComponentTypes();
	}

	WorldContext& GetContext() {
		return mContext;
	}

	void SetNextWorld(std::unique_ptr<World> world) {
		mNextWorld = std::move(world);
	}

	std::unique_ptr<World>& GetNextWorld() {
		return mNextWorld;
	}

	using ApplicationStateCreator =
		fu2::unique_function<std::unique_ptr<ApplicationState>(std::reference_wrapper<ApplicationStateContext>)>;
	void SetNextApplicationState(ApplicationStateCreator factoryFunc);
	auto GetNextApplicationState(ApplicationStateContext& context) -> std::unique_ptr<ApplicationState>;

private:

	void UpdateAudioComponents();

	std::chrono::duration<float> mTimestep = std::chrono::duration<float>(1.0f / 60.0f);

	int mStepCount = 0;

	bool mPaused = false;

	TimePoint mTotalTime = TimePoint(0.0f);

	int mMainCameraIndex = -1;

	EntityId mNextEntityId = EntityId(1);

	AmbientLight mAmbientLight;

	DirectionalLight mDirectionalLight;

	Fog mFog;

	AnimatorCollection mAnimators;

	WorldContext& mContext;

	std::unique_ptr<World>             mNextWorld;
	std::unique_ptr<b2World>           mPhysicsWorld;
	std::unique_ptr<b2ContactListener> mContactListener;
	std::unique_ptr<AudioLibrary>      mAudioLibrary;
	std::unique_ptr<TextureLibrary>    mTextureLibrary;

	std::vector<std::reference_wrapper<Camera3D>>        mCameras;
	std::vector<std::reference_wrapper<RenderComponent>> mDetachedRenderComponents;
	std::vector<std::reference_wrapper<AudioComponent>>  mAudioComponents;
	std::vector<std::reference_wrapper<WorldUiRenderer>>      mUiRenderers;

	std::unordered_map<EntityId, std::unique_ptr<Entity>> mEntities;

	CustomComponentUpdater m_CustomComponentUpdater;

	Sky mSky;

	RenderSettings mRenderSettings;

	ApplicationStateCreator mNextApplicationStateFactory;
};

}