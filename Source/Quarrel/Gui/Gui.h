#pragma once

#include <SFML/Graphics/Text.hpp>

struct CreateTextParams {
	sf::String const string;
	sf::Font const& font;
	int characterSize = 20;
	sf::Color color = sf::Color::White;
	sf::Color outlineColor = sf::Color::Transparent;
	float outlineThickness = 0.0f;

	CreateTextParams(sf::String const& string, sf::Font const& font)
		: string(string)
		, font(font)
	{}
};

auto CreateText(CreateTextParams const& params) -> sf::Text;

void AlignTextBottomRight(sf::Text& text, sf::Vector2i const position, sf::Vector2i const padding);
void AlignTextCentre(sf::Text& text, sf::Vector2i const position);