#include "PhysicsUtils.h"

#include <Box2D/Dynamics/b2Body.h>
#include <Box2D/Dynamics/b2World.h>

namespace qvr {

namespace Physics {

void b2BodyDeleter::operator()(b2Body* body) const {
	body->GetWorld()->DestroyBody(body);
}

}

}