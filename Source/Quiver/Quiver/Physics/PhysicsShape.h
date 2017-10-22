#pragma once

//#include <Box2D/Collision/Shapes/b2CircleShape.h>
//#include <Box2D/Collision/Shapes/b2ChainShape.h>
//#include <Box2D/Collision/Shapes/b2EdgeShape.h>
//#include <Box2D/Collision/Shapes/b2PolygonShape.h>

#include <json.hpp>

class b2Shape;

namespace qvr {

class PhysicsShape {
public:
	static nlohmann::json ToJson(const b2Shape& shape);

	// TODO: Look into std::variant/another alternative to dynamic allocation.
	static std::unique_ptr<b2Shape> FromJson(const nlohmann::json & j);

	static bool VerifyJson(const nlohmann::json & j);
};

// This is a nice idea but I don't have time to get it actually working.
//class ShapeVariant
//{
//	b2Shape::Type type;
//	union
//	{
//		b2CircleShape circleShape;
//		b2PolygonShape polygonShape;
//		b2EdgeShape edgeShape;
//		b2ChainShape chainShape;
//	};
//public:
//	ShapeVariant()
//		: type(b2Shape::Type::e_circle)
//	{
//		b2CircleShape shape;
//		shape.m_p = b2Vec2(0.0f, 0.0f);
//		shape.m_radius = 0.5f;
//
//		circleShape = shape;
//	}
//
//	ShapeVariant(const b2Shape& shape)
//		: type(shape.GetType())
//	{
//		switch (type)
//		{
//		case b2Shape::Type::e_circle:
//			circleShape = (b2CircleShape&)shape; break;
//		case b2Shape::Type::e_polygon:
//			polygonShape = (b2PolygonShape&)shape; break;
//		case b2Shape::Type::e_chain:
//			chainShape = (b2ChainShape&)shape; break;
//		case b2Shape::Type::e_edge:
//			edgeShape = (b2EdgeShape&)shape; break;
//		}
//	}
//
//	~ShapeVariant() {}
//
//	ShapeVariant& operator=(const b2Shape& shape)
//	{
//		type = shape.GetType();
//
//		switch (type)
//		{
//		case b2Shape::Type::e_circle:
//			circleShape = (b2CircleShape&)shape; break;
//		case b2Shape::Type::e_polygon:
//			polygonShape = (b2PolygonShape&)shape; break;
//		case b2Shape::Type::e_chain:
//			chainShape = (b2ChainShape&)shape; break;
//		case b2Shape::Type::e_edge:
//			edgeShape = (b2EdgeShape&)shape; break;
//		}
//
//		return *this;
//	}
//
//	const b2Shape& Get() const { return circleShape; }
//};

}