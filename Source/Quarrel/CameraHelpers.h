#pragma once

#include <Quiver/Graphics/Camera3D.h>

namespace qvr {
class World;
}

class CameraOwner {
public:
	CameraOwner(qvr::World& world, const qvr::Camera3D& camera);
	CameraOwner(CameraOwner&& other);

	~CameraOwner();

	// TODO: FromJson & ToJson. 
	// Can remove the primary camera code from Camera3D To/FromJson.

	qvr::Camera3D camera;

private:
	qvr::World& world;
};