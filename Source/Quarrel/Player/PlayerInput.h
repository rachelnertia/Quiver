#pragma once

#ifndef QUARREL_PLAYER_INPUT_H
#define QUARREL_PLAYER_INPUT_H

#include "Box2D/Common/b2Math.h"

namespace qvr
{

class RawInputDevices;

}

namespace Quarrel
{

b2Vec2 GetDirectionVector(const qvr::RawInputDevices& devices);

// Get a number [-1, 1] representing the current state of the rotation axis. Words.
float GetTurnAngle(qvr::RawInputDevices& devices);

}

#endif 