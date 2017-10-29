#pragma once

#include <chrono>
#include <vector>

#include <optional.hpp>

#include <Box2D/Common/b2Math.h>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Texture.hpp>

#include "json.hpp"

#include "Quiver/Animation/AnimationSystem.h"
#include "Quiver/Entity/CustomComponent/CustomComponentUpdater.h"
#include "Quiver/Entity/EntityPrefab.h"
#include "Quiver/Graphics/Fog.h"
#include "Quiver/Graphics/Light.h"
#include "Quiver/Graphics/RenderSettings.h"
#include "Quiver/Graphics/Sky.h"
#include "Quiver/Graphics/WorldRaycastRenderer.h"

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

bool                   SaveWorld(const World & world, const std::string filename);
std::unique_ptr<World> LoadWorld(const std::string filename, CustomComponentTypeLibrary& customComponentTypes);

class WorldController;

class World {
public:
	World(
		CustomComponentTypeLibrary& customComponentTypes);

	World(
		CustomComponentTypeLibrary& customComponentTypes,
		const nlohmann::json& j);

	~World();

	World(const World&) = delete;
	World(const World&&) = delete;

	World& operator=(const World&) = delete;
	World& operator=(const World&&) = delete;

	void TakeStep(qvr::RawInputDevices& inputDevices);

	// While paused is true, TakeStep will do nothing.
	void SetPaused(const bool paused);

	void RenderDebug(sf::RenderTarget& target, const Camera2D & camera);
	void Render3D(sf::RenderTarget& target, const Camera3D & camera);

	Entity* CreateEntity(const b2Shape & shape, const b2Vec2 & position, const float angle = 0.0f);
	Entity* CreateEntity(const nlohmann::json & json, const b2Transform* transform = nullptr);

	bool RemoveEntityImmediate(const Entity& entity);

	void GuiControls();
	void GuiPerformanceInfo();

	bool ToJson(nlohmann::json & j) const;

	static bool VerifyJson(const nlohmann::json & j);

	bool SetMainCamera(const Camera3D& camera);

	const Camera3D* GetMainCamera() const;

	bool RegisterCamera(const Camera3D& camera);
	bool UnregisterCamera(const Camera3D& camera);

	bool RegisterDetachedRenderComponent(const RenderComponent& renderComponent);
	bool UnregisterDetachedRenderComponent(const RenderComponent& renderComponent);

	bool RegisterAnimatorWithAltViews(const RenderComponent& renderComponent);
	bool UnregisterAnimatorWithAltViews(const RenderComponent& renderComponent);

	void UpdateDetachedRenderComponents(const Camera3D& camera);
	void UpdateAnimatorAltViews(const Camera3D& camera);

	bool RegisterAudioComponent(const AudioComponent& audioComponent);
	bool UnregisterAudioComponent(const AudioComponent& audioComponent);

	bool RegisterCustomComponent(const CustomComponent& customComponent);
	bool UnregisterCustomComponent(const CustomComponent& customComponent);

	inline float GetTimestep() const { return mTimestep; }

	inline const DirectionalLight& GetDirectionalLight() const { return mDirectionalLight; }

	inline const b2World* GetPhysicsWorld() const { return mPhysicsWorld.get(); }

	AnimationSystem& GetAnimationSystem() { return mAnimationSystem; }
	AudioLibrary&    GetAudioLibrary() { return *mAudioLibrary.get(); }
	TextureLibrary&  GetTextureLibrary() { return *mTextureLibrary.get(); }

	bool AddEntity(std::unique_ptr<Entity> entity);

	EntityPrefabContainer mEntityPrefabs;

	sf::Color groundColor = sf::Color::Yellow;
	sf::Color skyColor = sf::Color::Transparent;

	std::unique_ptr<World>& GetNextWorld() { return mNextWorld; }

	void SetNextWorld(std::unique_ptr<World>& nextWorld) { mNextWorld.reset(nextWorld.release()); }

	// TODO: Some kind of World clock thing with time_point
	using TimePoint = std::chrono::duration<float>;

	TimePoint GetTime() const { return mTotalTime; }

	const AmbientLight& GetAmbientLight() const { return mAmbientLight; }
	const Fog&          GetFog()          const { return mFog; }

	CustomComponentTypeLibrary& GetCustomComponentTypes() const {
		return m_CustomComponentTypes;
	}

private:

	void UpdateAudioComponents();

	float mTimestep = 1.0f / 60.0f;

	int mStepCount = 0;

	bool mPaused = false;

	TimePoint mTotalTime = TimePoint(0.0f);

	int mMainCameraIndex = -1;

	AmbientLight mAmbientLight;

	DirectionalLight mDirectionalLight;

	Fog mFog;

	AnimationSystem mAnimationSystem;

	std::unique_ptr<b2World>              mPhysicsWorld;
	std::unique_ptr<b2ContactListener>    mContactListener;
	std::unique_ptr<AudioLibrary>         mAudioLibrary;
	std::unique_ptr<TextureLibrary>       mTextureLibrary;

	std::vector<std::reference_wrapper<Camera3D>>        mCameras;
	std::vector<std::reference_wrapper<RenderComponent>> mDetachedRenderComponents;
	std::vector<std::reference_wrapper<RenderComponent>> mAnimatedWithAltViews;
	std::vector<std::reference_wrapper<AudioComponent>>  mAudioComponents;

	std::vector<std::unique_ptr<WorldController>> mControllers;
	std::vector<std::unique_ptr<Entity>> mEntities;

	std::unique_ptr<World> mNextWorld;

	CustomComponentTypeLibrary& m_CustomComponentTypes;
	CustomComponentUpdater m_CustomComponentUpdater;

	Sky mSky;

	RenderSettings mRenderSettings;

	WorldRaycastRenderer mRaycastRenderer;
};

// TODO: Decide what to do with this.
class WorldController
{
public:
	WorldController(World& world) : mWorld(world) {}

	virtual ~WorldController() {}

	WorldController(const WorldController&) = delete;
	WorldController(const WorldController&&) = delete;

	WorldController& operator=(const WorldController&) = delete;
	WorldController& operator=(const WorldController&&) = delete;

	virtual void OnStep() = 0;

	virtual void GuiControls() {}

protected:
	World& GetWorld() const { return mWorld; }

private:
	World& mWorld;

};

}