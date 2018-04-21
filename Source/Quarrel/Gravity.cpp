#include "Gravity.h"

#include <algorithm>

#include <Quiver/Entity/RenderComponent/RenderComponent.h>

void UpdateGroundOffset(
	qvr::RenderComponent & graphicsComponent, 
	const float positionDelta)
{
	float groundOffset = graphicsComponent.GetGroundOffset();

	groundOffset =
		std::max(
			0.0f,
			groundOffset + positionDelta);

	graphicsComponent.SetGroundOffset(groundOffset);
}
