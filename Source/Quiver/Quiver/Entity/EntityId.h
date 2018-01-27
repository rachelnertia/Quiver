#pragma once

#include <named_type.hpp>

namespace qvr {

using EntityId = fluent::NamedType<int, struct EntityIdTag, fluent::Comparable, fluent::Hashable>;

}