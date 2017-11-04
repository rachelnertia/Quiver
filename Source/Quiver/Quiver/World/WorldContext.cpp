#include "WorldContext.h"

#include "Quiver/Application/ApplicationState.h"
#include "Quiver/World/World.h"

namespace qvr
{

void WorldContext::SetNextApplicationState(std::unique_ptr<ApplicationState> nextState) {
	mNextApplicationState = std::move(nextState);
}

std::unique_ptr<ApplicationState>& WorldContext::GetNextApplicationState() {
	return mNextApplicationState;
}

WorldContext::~WorldContext() {};

}