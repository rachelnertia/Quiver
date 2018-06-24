#pragma once

#include <json.hpp>

#include <SFML/Graphics/Color.hpp>

namespace ColourUtils {

unsigned HexStringToUint(const std::string& str);

std::string UintToHexString(const unsigned number);

nlohmann::json ToJson(const sf::Color col);

bool SerializeSFColorToJson(const sf::Color col, nlohmann::json& j);

bool DeserializeSFColorFromJson(sf::Color& col, const nlohmann::json& j);

bool VerifyColourJson(const nlohmann::json& j);

void ImGuiColourEdit(const char* label, sf::Color& colour);
void ImGuiColourEditRGB(const char* label, sf::Color & colour);

}

namespace sf {

inline void to_json(nlohmann::json& j, const Color& color) {
	j = ColourUtils::ToJson(color);
}

inline void from_json(const nlohmann::json& j, Color& color) {
	ColourUtils::DeserializeSFColorFromJson(color, j);
}

}