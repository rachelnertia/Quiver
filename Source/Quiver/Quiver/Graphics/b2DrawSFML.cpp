#include "b2DrawSFML.h"

#include "SFML/Graphics/RenderTarget.hpp"
#include "SFML/Graphics/ConvexShape.hpp"
#include "SFML/Graphics/CircleShape.hpp"

inline sf::Color B2ColorToSFColor(const b2Color& color) {
	return sf::Color(sf::Uint8(color.r * 255), sf::Uint8(color.g * 255),
		sf::Uint8(color.b * 255), sf::Uint8(color.a * 255));
}

inline sf::Vector2f B2VecToSFVec(const b2Vec2& b2vec) {
	return sf::Vector2f(b2vec.x, b2vec.y);
}

void b2DrawSFML::DrawPolygon(const b2Vec2 * vertices, int32 vertexCount, const b2Color & color)
{
	sf::ConvexShape polygon(vertexCount);
	for (int i = 0; i < vertexCount; ++i) {
		sf::Vector2f pos = B2VecToSFVec(mCamera.WorldToCamera(vertices[i]));
		polygon.setPoint(i, pos);
	}
	polygon.setOutlineThickness(-1.0f);
	polygon.setFillColor(sf::Color::Transparent);
	polygon.setOutlineColor(B2ColorToSFColor(color));

	mTarget->draw(polygon);
}

void b2DrawSFML::DrawSolidPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color) {
	sf::ConvexShape polygon(vertexCount);
	for (int i = 0; i < vertexCount; ++i) {
		sf::Vector2f pos = B2VecToSFVec(mCamera.WorldToCamera(vertices[i]));
		polygon.setPoint(i, pos);
	}
	polygon.setOutlineThickness(-1.0f);
	polygon.setOutlineColor(B2ColorToSFColor(color));
	sf::Color c = B2ColorToSFColor(color);
	c.a /= 2;
	polygon.setFillColor(c);

	mTarget->draw(polygon);
}

void b2DrawSFML::DrawCircle(const b2Vec2& center, float32 radius, const b2Color& color) {
	float scaled_radius = mCamera.MetresToPixels(radius);
	sf::CircleShape circle(scaled_radius);
	circle.setOrigin(scaled_radius, scaled_radius);
	circle.setPosition(B2VecToSFVec(mCamera.WorldToCamera(center)));
	circle.setOutlineThickness(-1.0f);
	circle.setOutlineColor(B2ColorToSFColor(color));

	mTarget->draw(circle);
}

void b2DrawSFML::DrawSolidCircle(const b2Vec2& center, float32 radius, const b2Vec2& axis, const b2Color& color) {
	float scaled_radius = mCamera.MetresToPixels(radius);
	sf::CircleShape circle(scaled_radius);
	circle.setOrigin(scaled_radius, scaled_radius);
	circle.setPosition(B2VecToSFVec(mCamera.WorldToCamera(center)));
	circle.setOutlineThickness(-1.0f);
	circle.setOutlineColor(B2ColorToSFColor(color));
	sf::Color c = B2ColorToSFColor(color);
	c.a /= 2;
	circle.setFillColor(c);

	b2Vec2 endPoint = center + radius * axis;
	sf::Vertex line[2] =
	{
		sf::Vertex(B2VecToSFVec(mCamera.WorldToCamera(center)), B2ColorToSFColor(color)),
		sf::Vertex(B2VecToSFVec(mCamera.WorldToCamera(endPoint)), B2ColorToSFColor(color)),
	};

	mTarget->draw(circle);
	mTarget->draw(line, 2, sf::Lines);
}

void b2DrawSFML::DrawSegment(const b2Vec2& p1, const b2Vec2& p2, const b2Color& color) {
	sf::Vertex line[] =
	{
		sf::Vertex(B2VecToSFVec(mCamera.WorldToCamera(p1)), B2ColorToSFColor(color)),
		sf::Vertex(B2VecToSFVec(mCamera.WorldToCamera(p2)), B2ColorToSFColor(color))
	};

	mTarget->draw(line, 2, sf::Lines);
}

void b2DrawSFML::DrawTransform(const b2Transform& xf) {
	float linelength = 0.4f;

	b2Vec2 xAxis = xf.p + linelength * xf.q.GetXAxis();
	sf::Vertex redLine[] =
	{
		sf::Vertex(B2VecToSFVec(mCamera.WorldToCamera(xf.p)), sf::Color::Red),
		sf::Vertex(B2VecToSFVec(mCamera.WorldToCamera(xAxis)), sf::Color::Red)
	};

	b2Vec2 yAxis = xf.p + linelength * xf.q.GetYAxis();
	sf::Vertex greenLine[] =
	{
		sf::Vertex(B2VecToSFVec(mCamera.WorldToCamera(xf.p)), sf::Color::Green),
		sf::Vertex(B2VecToSFVec(mCamera.WorldToCamera(yAxis)), sf::Color::Green)
	};

	mTarget->draw(redLine, 2, sf::Lines);
	mTarget->draw(greenLine, 2, sf::Lines);
}