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
	bool mWindowResized = false;

	friend int RunApplication(CustomComponentTypeLibrary&);

public:
	ApplicationStateContext(
		sf::RenderWindow& window,
		CustomComponentTypeLibrary& customComponentTypes)
		: mWindow(window)
		, mCustomComponentTypes(customComponentTypes)
	{}

	sf::RenderWindow&           GetWindow() { return mWindow; }
	bool                        WindowResized() { return mWindowResized; }
	CustomComponentTypeLibrary& GetCustomComponentTypes() { return mCustomComponentTypes; }
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