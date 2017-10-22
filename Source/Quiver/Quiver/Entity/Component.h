#pragma once

namespace qvr {

class Entity;

class Component {
public:
	explicit Component(Entity& entity) : mEntity(entity) {}

	virtual ~Component() {}

	Component(const Component&) = delete;
	Component(const Component&&) = delete;

	Component& operator=(const Component&) = delete;
	Component& operator=(const Component&&) = delete;

	Entity& GetEntity() const { return mEntity; }

private:
	Entity& mEntity;
};

}