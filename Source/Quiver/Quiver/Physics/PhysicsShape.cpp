#include "PhysicsShape.h"

#include <Box2D/Collision/Shapes/b2CircleShape.h>
#include <Box2D/Collision/Shapes/b2PolygonShape.h>

#include "Quiver/Misc/Logging.h"

namespace qvr {

nlohmann::json PhysicsShape::ToJson(const b2Shape& shape)
{
	auto log = GetConsoleLogger();

	b2Shape::Type shapeType = shape.GetType();

	if ((shapeType < 0) || (shapeType >= b2Shape::Type::e_typeCount)) {
		log->error("Shape type is invalid.");
		return {};
	}

	//assert(shapeType >= 0);
	//assert(shapeType < b2Shape::Type::e_typeCount);

	nlohmann::json j;

	std::string shapeTypeString;
	switch (shapeType) {
	case b2Shape::Type::e_circle:
		shapeTypeString = "Circle";
		break;
	case b2Shape::Type::e_edge:
		//shapeTypeString = "Edge"; // Can't make in WorldEditor, yet.
		//log->error("We don't support Edge shapes yet!");
		return {};
		break;
	case b2Shape::Type::e_polygon:
		shapeTypeString = "Polygon";
		break;
	case b2Shape::Type::e_chain:
		//shapeTypeString = "Chain"; // Can't make in WorldEditor, yet.
		//log->error("We don't support Chain shapes yet!");
		return {};
		break;
	default:
		//shapeTypeString = "Invalid";
		return {};
		break;
	}

	j["Type"] = shapeTypeString;

	// Will just do Circle and Polygon for now.

	if (shapeType == b2Shape::Type::e_circle) {
		b2CircleShape& circleShape = (b2CircleShape&)shape;

		j["Radius"] = fabsf(circleShape.m_radius);

	}
	else if (shapeType == b2Shape::Type::e_polygon) {
		b2PolygonShape& polygonShape = (b2PolygonShape&)shape;

		int vertexCount = polygonShape.GetVertexCount();

		if ((vertexCount < 3) || (vertexCount > b2_maxPolygonVertices)) {
			log->error("Polygon shape vertex count invalid.");
			return {};
		}

		for (int i = 0; i < vertexCount; ++i) {
			b2Vec2 vertex = polygonShape.GetVertex(i);
			j["Vertices"][i][0] = vertex.x;
			j["Vertices"][i][1] = vertex.y;
		}

		// TODO: Find out if we need the normals too!
	}

	return j;
}

// This function allocates memory!
// Yes, that's a reference to a pointer.
std::unique_ptr<b2Shape> PhysicsShape::FromJson(const nlohmann::json & j)
{
	std::string typeString = j["Type"];

	if (typeString == "Circle") {
		auto shape = std::make_unique<b2CircleShape>();
		b2CircleShape* circle = static_cast<b2CircleShape*>(shape.get());
		circle->m_radius = j["Radius"];
		return shape;
	}
	else if (typeString == "Polygon") {
		auto shape = std::make_unique<b2PolygonShape>();
		b2PolygonShape* polygon = static_cast<b2PolygonShape*>(shape.get());
		std::vector<b2Vec2> verts;
		for (auto& v : j["Vertices"]) {
			verts.push_back(b2Vec2(v[0], v[1]));
		}
		polygon->Set(&verts[0], verts.size());
		return shape;
	}

	return nullptr;
}

}