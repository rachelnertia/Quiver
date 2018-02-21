#pragma once

#include <memory>

#include "Quiver/World/WorldContext.h"

namespace sf {
class RenderWindow;
class Event;
}

namespace qvr {

class CustomComponentTypeLibrary;

// Contains data shared by multiple ApplicationStates.
class ApplicationStateContext {
	sf::RenderWindow& mWindow;
	bool mWindowResized = false;
	
	WorldContext mWorldContext;

	friend int RunApplication(
		CustomComponentTypeLibrary&, 
		FixtureFilterBitNames&);

public:
	ApplicationStateContext(
		sf::RenderWindow& window,
		CustomComponentTypeLibrary& customComponentTypes,
		FixtureFilterBitNames& filterBitNames)
		: mWindow(window)
		, mWorldContext(
			customComponentTypes, 
			filterBitNames)
	{}

	sf::RenderWindow&      GetWindow() { return mWindow; }
	bool                   WindowResized() { return mWindowResized; }
	WorldContext&          GetWorldContext() { return mWorldContext; }
};

class ApplicationState {
public:
	ApplicationState(ApplicationStateContext& context) : mContext(context) {}
	
	virtual ~ApplicationState() = default;

	ApplicationState(const ApplicationState& other) = delete;
	ApplicationState(const ApplicationState&& other) = delete;

	ApplicationState& operator=(const ApplicationState& other) = delete;
	ApplicationState& operator=(const ApplicationState&& other) = delete;

	virtual void ProcessFrame() = 0;

	bool GetQuit() const { return mQuit; }

	std::unique_ptr<ApplicationState>& GetNextState() { return mNextState; }

protected:
	ApplicationStateContext& GetContext() { return mContext; }

	void SetQuit(std::unique_ptr<ApplicationState> nextState) 
	{ 
		mQuit = true;
		mNextState.swap(nextState);
	}

private:
	ApplicationStateContext& mContext;
	
	std::unique_ptr<ApplicationState> mNextState;
	
	bool mQuit = false;
};

}