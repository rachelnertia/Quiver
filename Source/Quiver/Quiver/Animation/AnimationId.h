#pragma once

#include <functional>
#include <ostream>

namespace qvr {

class AnimationId
{
public:
	explicit AnimationId(const unsigned value) : mValue(value) {}

	AnimationId() = default;

	static const AnimationId Invalid;

	unsigned GetValue() const { return mValue; }

private:
	unsigned mValue = AnimationId::Invalid.GetValue();
};

inline bool operator==(const AnimationId lhs, const AnimationId rhs) { return lhs.GetValue() == rhs.GetValue(); }
inline bool operator!=(const AnimationId lhs, const AnimationId rhs) { return lhs.GetValue() != rhs.GetValue(); }

inline std::ostream& operator<<(std::ostream& lhs, const AnimationId rhs) { return lhs << rhs.GetValue(); }

}

namespace std
{
template <>
struct hash<qvr::AnimationId>
{
	std::size_t operator()(const qvr::AnimationId id) const
	{
		return hash<unsigned>{}(id.GetValue());
	}
};
}
