#include "CameraHelpers.h"

#include <Quiver/World/World.h>

CameraOwner::CameraOwner(qvr::World& world, const qvr::Camera3D& camera)
	: world(world)
	, camera(camera)
{
	world.RegisterCamera(camera);
}

CameraOwner::CameraOwner(CameraOwner&& other)
	: camera(other.camera)
	, world(other.world)
{
	world.RegisterCamera(camera);

	if (world.GetMainCamera() == &other.camera) {
		world.SetMainCamera(camera);
	}

	world.UnregisterCamera(other.camera);
}

CameraOwner::~CameraOwner() {
	world.UnregisterCamera(camera);
}