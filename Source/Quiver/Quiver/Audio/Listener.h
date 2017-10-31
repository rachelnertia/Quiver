#pragma once

struct b2Vec2;

namespace qvr
{

class Camera3D;

void UpdateListener(const b2Vec2& position, const b2Vec2& direction);
void UpdateListener(const Camera3D& camera);

}