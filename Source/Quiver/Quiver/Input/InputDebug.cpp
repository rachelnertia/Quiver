#include "InputDebug.h"

#include <ImGui/imgui.h>
#include <SFML/Window/Joystick.hpp>
#include <spdlog/fmt/fmt.h>

#include "SfmlJoystick.h"

namespace qvr {

void SfmlJoystickDebugGui(const SfmlJoystick& joystick, const int joystickIndex);

void SfmlJoystickDebugGui(const SfmlJoystickSet& joysticks) {
	for (int joystickIndex = 0; joystickIndex < SfmlJoystickSet::Count; ++joystickIndex)
	{
		const auto joystick = (SfmlJoystick*)joysticks.GetJoystick(JoystickIndex(joystickIndex));

		if (joystick == nullptr) continue;
		
		if (ImGui::CollapsingHeader(fmt::format("Joystick {}", joystickIndex).c_str())) {
			SfmlJoystickDebugGui(*joystick, joystickIndex);
		}
	}
}

bool IsXbox360Controller(const sf::Joystick::Identification& id) {
	/*if (id.name == "Controller (XBOX 360 For Windows)") {
		return true;
	}*/

	if ((id.productId == 654 || id.productId == 673) &&
		id.vendorId == 1118)
	{
		return true;
	}

	return false;
}

const char* Xbox360ControllerButtonStr(const int buttonIndex) {
	std::array<const char*, 10> strings = {
		"A", 
		"B",
		"X",
		"Y",
		"Left Bumper",
		"Right Bumper",
		"Back",
		"Start",
		"Left Stick Click",
		"Right Stick Click"
	};

	return strings.at(buttonIndex);
}

const char* SfmlAxisStr(const sf::Joystick::Axis axis) {
	std::array<const char*, sf::Joystick::AxisCount> strings = {
		"X",
		"Y",
		"Z",
		"R",
		"U",
		"V",
		"PovX",
		"PovY"
	};

	return strings.at((int)axis);
}

const char* Xbox360ControllerAxisStr(const sf::Joystick::Axis axis) {
	std::array<const char*, sf::Joystick::AxisCount> strings = {
		"Left Stick Horizontal",
		"Left Stick Vertical",
		"Triggers",
		"Right Stick Vertical",
		"Right Stick Horizontal",
		"?",
		"D-Pad Horizontal",
		"D-Pad Vertical"
	};

	return strings.at(axis);
}

void SfmlJoystickDebugGui(const SfmlJoystick& joystick, const int joystickIndex) {
	bool isXbox360Controller = false;
	
	{
		using Identification = sf::Joystick::Identification;

		const Identification identification =
			sf::Joystick::getIdentification(joystickIndex);

		isXbox360Controller = IsXbox360Controller(identification);

		ImGui::Text("Name: %s", identification.name.toAnsiString().c_str());
		ImGui::Text("Product ID: %u", identification.productId);
		ImGui::Text("Vendor ID: %u", identification.vendorId);
	}

	if (ImGui::Button(fmt::format("Log Info ##{}", joystickIndex).c_str())) {
		LogSfmlJoystickInfo(joystickIndex);
	}

	if (ImGui::CollapsingHeader(fmt::format("Buttons ##{}", joystickIndex).c_str())) {

		ImGui::Text("Count: %d", joystick.GetButtonCount());

		auto buttons = joystick.GetButtons();

		int buttonIndex = 0;

		for (auto buttonState : buttons) {
			const char* name = "";

			if (isXbox360Controller) {
				name = Xbox360ControllerButtonStr(buttonIndex);
			}

			ImGui::Text("%d: %s", buttonIndex, name);

			ImGui::SameLine(ImGui::GetWindowWidth() / 3.0f);

			ImGui::RadioButton("Is Down", buttonState.isDown);

			ImGui::SameLine((ImGui::GetWindowWidth() / 3.0f) * 2.0f);

			ImGui::RadioButton("Was Down", buttonState.wasDown);

			buttonIndex++;
		}
	}

	if (ImGui::CollapsingHeader("Axes")) {
		ImGui::Text("Count: %d", joystick.GetAxisCount());

		auto axes = joystick.GetAxes();

		int axisIndex = 0;

		for (auto axisState : axes) {
			if (sf::Joystick::hasAxis(joystickIndex, (sf::Joystick::Axis)axisIndex))
			{
				const char* name = 
					isXbox360Controller ? 
					Xbox360ControllerAxisStr((sf::Joystick::Axis)axisIndex) :
					SfmlAxisStr((sf::Joystick::Axis)axisIndex);

				ImGui::SliderFloat(
					fmt::format("{}: {}", axisIndex, name).c_str(), 
					&axisState.position, 
					-100.0f, 
					100.0f);
			}

			axisIndex++;
		}
	}
}

}