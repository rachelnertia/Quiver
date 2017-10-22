#pragma once

#include "json.hpp"

namespace qvr {

namespace Animation {

struct Rect {
	int top = 0, left = 0, bottom = 0, right = 0;

	bool FromJson(const nlohmann::json& j);

	bool ToJson(nlohmann::json& j) const;

	struct Dimensions {
		int width, height;
	};
};

inline int GetWidth(const Rect& rect) {
	return abs(rect.right - rect.left);
}

inline int GetHeight(const Rect& rect) {
	return abs(rect.bottom - rect.top);
}

inline bool operator==(const Rect& a, const Rect& b) {
	return
		a.top == b.top    &&
		a.bottom == b.bottom &&
		a.left == b.left   &&
		a.right == b.right;
}

inline bool operator!=(const Rect& a, const Rect& b) {
	return !(a == b);
}

inline Rect::Dimensions GetDimensions(const Rect& rect) {
	Rect::Dimensions ret;
	ret.width = GetWidth(rect);
	ret.height = GetHeight(rect);
	return ret;
}

inline bool operator==(const Rect::Dimensions& a, const Rect::Dimensions& b) {
	return a.width == b.width && a.height == b.height;
}

inline bool operator!=(const Rect::Dimensions& a, const Rect::Dimensions& b) {
	return !(a == b);
}

}

}