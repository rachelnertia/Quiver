#include "WorldUiRenderer.h"

#include "Quiver/World/World.h"

namespace qvr {

WorldUiRenderer::WorldUiRenderer(World& world, RenderFunction renderFunction)
	: world(world)
	, renderFunction(renderFunction)
{
	world.RegisterUiRenderer(*this);
}

WorldUiRenderer::~WorldUiRenderer() {
	world.UnregisterUiRenderer(*this);
}

}