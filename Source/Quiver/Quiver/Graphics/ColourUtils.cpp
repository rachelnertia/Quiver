#include "ColourUtils.h"
#include <sstream>
#include <ImGui/imgui.h>
#include <SFML/Graphics/Color.hpp>

namespace ColourUtils {

unsigned HexStringToUint(const std::string& str) {
	std::stringstream converter(str);
	unsigned value = 0;
	converter >> std::hex >> value;
	return value;
}

std::string UintToHexString(const unsigned number) {
	std::stringstream stream;
	stream << std::setfill('0') << std::setw(8) << std::hex << number;
	return stream.str();
}

nlohmann::json ToJson(const sf::Color col) {
	return nlohmann::json(UintToHexString(col.toInteger()));
}

bool SerializeSFColorToJson(const sf::Color col, nlohmann::json& j) {
	if (!j.empty()) return false;
	j = ToJson(col);
	return true;
}

bool DeserializeSFColorFromJson(sf::Color& col, const nlohmann::json& j) {
	if (j.is_string()) {
		col = sf::Color(HexStringToUint(j.get<std::string>()));
		return true;
	}
	
	if (j.is_number_unsigned()) {
		col = sf::Color(j.get<unsigned>());
		return true;
	}

	return false;
}

bool VerifyColourJson(const nlohmann::json& j) {
	if (!j.is_string() && !j.is_number_unsigned()) {
		return false;
	}

	// Check validity of Colour hex string
	if (j.is_string()) {
		std::string hexString = j.get<std::string>();
		if (hexString.length() != 8) {
			std::cout << hexString << " not a valid hex string. Must be 8 characters in length." << std::endl;
			return false;
		}
	}

	return true;
}

void ImGuiColourEdit(const char* label, sf::Color & colour)
{
	const float s = (1.0f / 255.0f);
	ImVec4 fcolor(colour.r * s, colour.g * s, colour.b * s, colour.a * s);
	if (ImGui::ColorEdit4(label , &fcolor.x)) {
		colour = sf::Color((sf::Uint8)(fcolor.x * 255),
			               (sf::Uint8)(fcolor.y * 255),
			               (sf::Uint8)(fcolor.z * 255),
			               (sf::Uint8)(fcolor.w * 255));
	}
}

void ImGuiColourEditRGB(const char* label, sf::Color & colour)
{
	const float s = (1.0f / 255.0f);
	ImVec4 fcolor(colour.r * s, colour.g * s, colour.b * s, colour.a * s);
	if (ImGui::ColorEdit3(label, &fcolor.x)) {
		colour = sf::Color((sf::Uint8)(fcolor.x * 255),
			(sf::Uint8)(fcolor.y * 255),
			(sf::Uint8)(fcolor.z * 255),
			(sf::Uint8)(fcolor.w * 255));
	}
}

}