#pragma once

#include <memory>

namespace sf {
class RenderWindow;
class Event;
}

namespace qvr {

class CustomComponentTypeLibrary;

class ApplicationStateContext {
	sf::RenderWindow& mWindow;
	CustomComponentTypeLibrary& mCustomComponentTypes;
public:
	ApplicationStateContext(
		sf::RenderWindow& window,
		CustomComponentTypeLibrary& customComponentTypes)
		: mWindow(window)
		, mCustomComponentTypes(customComponentTypes)
	{}

	sf::RenderWindow&           GetWindow() { return mWindow; }
	CustomComponentTypeLibrary& GetCustomComponentTypes() { return mCustomComponentTypes; }
};

class ApplicationState {
public:
	ApplicationState(ApplicationStateContext& context) : mContext(context) {}

	ApplicationState(const ApplicationState& other) = delete;
	ApplicationState(const ApplicationState&& other) = delete;

	ApplicationState& operator=(const ApplicationState& other) = delete;
	ApplicationState& operator=(const ApplicationState&& other) = delete;

	virtual void ProcessEvent(sf::Event& event) = 0;
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