#pragma once

#include <vector>

template<typename T>
auto FindByAddress(const std::vector<std::reference_wrapper<T>>& v, const T& tRef)
{
	for (auto it = v.begin(); it != v.end(); ++it)
	{
		const auto deref = *it;

		if (&deref.get() == &tRef)
		{
			return it;
		}
	}

	return v.end();
}
