#include "MainMenu.h"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Event.hpp>

#include "ImGui/imgui.h"

#include "Quiver/Application/WorldEditor/WorldEditor.h"
#include "Quiver/Application/Game/Game.h"
#include "Quiver/Misc/ImGuiHelpers.h"
#include "Quiver/World/World.h"

namespace qvr {

void MainMenu::ProcessEvent(sf::Event & event)
{
	if (GetContext().GetWindow().hasFocus()) {
		switch (event.type) {
		case sf::Event::MouseButtonPressed:
			break;
		case sf::Event::MouseMoved:
			break;
		case sf::Event::Resized:
			break;
		default:
			break;
		}
	}
}

void MainMenu::ProcessFrame()
{
	{
		ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
		ImGui::SetNextWindowSize(ImVec2(400.0f, (float)GetContext().GetWindow().getSize().y));

		ImGui::AutoWindow window(
			"Main Menu",
			nullptr,
			ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

		ImGui::AutoStyleVar style(ImGuiStyleVar_IndentSpacing, 5.0f);

		if (ImGui::Button("Play"))
		{
			SetQuit(std::make_unique<Game>(GetContext()));
		}

		if (ImGui::Button("Editor"))
		{
			SetQuit(std::make_unique<WorldEditor>(GetContext()));
		}
	}

	GetContext().GetWindow().clear(sf::Color(128, 128, 255));

	ImGui::Render();

	GetContext().GetWindow().display();
}

}