#include "Rect.h"

namespace qvr {

using namespace Animation;

bool Rect::FromJson(const nlohmann::json & j) {
	if (j.find("top") == j.end()
		|| j.find("bottom") == j.end()
		|| j.find("left") == j.end()
		|| j.find("right") == j.end())
	{
		return false;
	}

	if (!j["top"].is_number_integer()
		|| !j["bottom"].is_number_integer()
		|| !j["left"].is_number_integer()
		|| !j["right"].is_number_integer())
	{
		return false;
	}

	top = j["top"];
	bottom = j["bottom"];
	right = j["right"];
	left = j["left"];

	return true;
}

bool Rect::ToJson(nlohmann::json & j) const
{
	if (!j.empty()) { return false; }

	j["top"] = top;
	j["bottom"] = bottom;
	j["right"] = right;
	j["left"] = left;

	return true;
}

}