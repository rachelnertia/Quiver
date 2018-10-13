#include "Gui.h"

sf::Text CreateText(CreateTextParams const& params) {
	sf::Text text;
	text.setFont(params.font);
	text.setString(params.string);
	text.setCharacterSize(params.characterSize);
	text.setOutlineColor(params.outlineColor);
	text.setFillColor(params.color);
	text.setOutlineThickness(params.outlineThickness);
	return text;
}

void AlignTextBottomRight(sf::Text& text, sf::Vector2i const position, sf::Vector2i const padding)
{
	float const alignRight = (float)position.x - padding.x;
	float const alignBottom = (float)position.y - padding.y;
	sf::FloatRect const bounds = text.getLocalBounds();
	text.setPosition(
		alignRight - bounds.width - bounds.left,
		alignBottom - bounds.height - bounds.top);
}

void AlignTextCentre(sf::Text& text, sf::Vector2i const position) {
	sf::FloatRect const bounds = text.getLocalBounds();
	text.setPosition(
		position.x - (bounds.width - bounds.left) / 2, 
		position.y - (bounds.height - bounds.top) / 2);
}