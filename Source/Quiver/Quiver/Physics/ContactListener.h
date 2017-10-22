#pragma once

#include "Box2D/Dynamics/b2WorldCallbacks.h"

class Entity;

namespace qvr {

namespace Physics
{

class ContactListener : public b2ContactListener
{
public:
	void BeginContact(b2Contact* contact) override;
	void EndContact(b2Contact* contact) override;
};

}

}