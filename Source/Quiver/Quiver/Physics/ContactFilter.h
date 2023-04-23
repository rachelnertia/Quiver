#pragma once

#include "Box2D/Dynamics/b2WorldCallbacks.h"

namespace qvr {

namespace Physics {

class ContactFilter : public b2ContactFilter
{
public:
	bool ShouldCollide(b2Fixture* fixtureA, b2Fixture* fixtureB) override;
};

}

}