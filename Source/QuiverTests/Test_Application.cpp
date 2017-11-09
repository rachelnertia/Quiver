#include <memory>

#include <catch.hpp>
#include <SFML/Graphics/RenderWindow.hpp>

#include "Quiver/Application/ApplicationState.h"
#include "Quiver/Entity/CustomComponent/CustomComponent.h"

class TestApplicationState : public qvr::ApplicationState
{
public:
	TestApplicationState(qvr::ApplicationStateContext& context)
		: qvr::ApplicationState(context)
	{}

	void ProcessEvent(sf::Event&) override {}
	void ProcessFrame() override
	{
		this->SetQuit(
			std::make_unique<TestApplicationState>(this->GetContext()));
	}
};

TEST_CASE("ApplicationState", "[Application]")
{
	sf::RenderWindow window;
	qvr::CustomComponentTypeLibrary customComponentTypes;

	qvr::ApplicationStateContext context(window, customComponentTypes);

	std::unique_ptr<qvr::ApplicationState> applicationState =
		std::make_unique<TestApplicationState>(context);

	applicationState->ProcessFrame();

	REQUIRE(applicationState->GetQuit() == true);
	REQUIRE(applicationState->GetNextState() != nullptr);

	applicationState = std::move(applicationState->GetNextState());

	REQUIRE(applicationState != nullptr);
	REQUIRE(applicationState->GetQuit() == false);
	REQUIRE(applicationState->GetNextState() == nullptr);
}