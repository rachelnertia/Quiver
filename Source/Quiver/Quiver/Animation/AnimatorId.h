#pragma once

#include <functional>
#include <ostream>

namespace qvr {

class AnimatorId
{
public:
	explicit AnimatorId(const unsigned value) : mValue(value) {}

	static const AnimatorId Invalid;

	unsigned GetValue() const { return mValue; }

private:
	unsigned mValue;
};

inline bool operator==(const AnimatorId lhs, const AnimatorId rhs) { return lhs.GetValue() == rhs.GetValue(); }
inline bool operator!=(const AnimatorId lhs, const AnimatorId rhs) { return lhs.GetValue() != rhs.GetValue(); }

inline std::ostream& operator<<(std::ostream& lhs, const AnimatorId rhs) { return lhs << rhs.GetValue(); }

}

namespace std
{
template <>
struct hash<qvr::AnimatorId>
{
	std::size_t operator()(const qvr::AnimatorId id) const
	{
		return hash<unsigned>{}(id.GetValue());
	}
};
}