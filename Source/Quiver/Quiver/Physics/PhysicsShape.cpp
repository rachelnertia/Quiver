#include "PhysicsShape.h"
#include <iostream>
#include <Box2D/Collision/Shapes/b2CircleShape.h>
#include <Box2D/Collision/Shapes/b2PolygonShape.h>

namespace qvr {

nlohmann::json PhysicsShape::ToJson(const b2Shape& shape)
{
	b2Shape::Type shapeType = shape.GetType();

	if ((shapeType < 0) || (shapeType >= b2Shape::Type::e_typeCount)) {
		std::cout << "Shape type is invalid." << std::endl;
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
		//std::cout << "We don't support Edge shapes yet!" << std::endl;
		return {};
		break;
	case b2Shape::Type::e_polygon:
		shapeTypeString = "Polygon";
		break;
	case b2Shape::Type::e_chain:
		//shapeTypeString = "Chain"; // Can't make in WorldEditor, yet.
		//std::cout << "We don't support Chain shapes yet!" << std::endl;
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
			std::cout << "Polygon shape vertex count invalid." << std::endl;
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
	// How much validation should this function have in it?
	// Should we assume that VerifyJson has already been run and
	// not execute it here?

	if (!VerifyJson(j)) {
		std::cout << "JSON is not valid for a shape." << std::endl;
		return nullptr;
	}

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

bool PhysicsShape::VerifyJson(const nlohmann::json & j)
{
	if (!j.is_object()) return false;

	if (j.find("Type") == j.end()) {
		return false;
	}

	if (!j["Type"].is_string()) {
		return false;
	}

	std::string typeString = j["Type"];

	// Check that typeString is valid.
	{
		const std::string validTypeStrings[] = {
			"Circle",
			"Polygon"
		};
		bool typeStringIsValid = false;
		for (auto& validTypeString : validTypeStrings) {
			if (typeString == validTypeString) {
				typeStringIsValid = true;
				break;
			}
		}
		if (!typeStringIsValid) {
			return false;
		}
	}

	// Now we do different things depending on value of typeString.
	if (typeString == "Circle") {
		// Check there's a radius.
		if (j.find("Radius") == j.end()) return false;
		if (!j["Radius"].is_number()) return false;

	}
	else if (typeString == "Polygon") {
		// Check for vertices.
		if (j.find("Vertices") == j.end()) return false;
		auto& vertices = j["Vertices"];
		// Check that vertices is an array.
		if (!vertices.is_array()) return false;
		// Check that there are at least 3 verts.
		if (vertices.size() < 3) return false;
		// Check that there are no more than the maximum number of verts.
		// TODO
		// Check that each vertex is an array with 2 elements, each of which
		// is a number.
		for (auto& vertex : j["Vertices"]) {
			if (!vertex.is_array()) return false;
			if (vertex.size() != 2) return false;
			if (!vertex[0].is_number()) return false;
			if (!vertex[1].is_number()) return false;
		}
		// Phew!

	}
	else {
		// Shouldn't reach here, but if we do, something's deffo borked.
		return false;
	}

	return true;
}

}