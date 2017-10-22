#pragma once

#include <json.hpp>

namespace sf {
class Color;
}

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