#pragma once

#include <memory>

namespace qvr
{

class ApplicationState; 
class World;

// This is used to communicate from the World to its owner/controller.
class WorldContext
{
public:
	// Flag that an object in the World would like to change the ApplicationState.
	void SetNextApplicationState(std::unique_ptr<ApplicationState> nextState);

	std::unique_ptr<ApplicationState>& GetNextApplicationState();

	// Flag that an object in the World would like to change the World.
	void SetNextWorld(std::unique_ptr<World> nextWorld) {
		mNextWorld = std::move(nextWorld);
	}

	std::unique_ptr<World>& GetNextWorld() { 
		return mNextWorld; 
	}

	~WorldContext();

private:
	std::unique_ptr<ApplicationState> mNextApplicationState;
	std::unique_ptr<World>            mNextWorld;
};

}