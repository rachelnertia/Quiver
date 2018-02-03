#pragma once

#include "Quiver/Entity/Component.h"

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <json.hpp>

class b2Fixture;

namespace sf {
class Window;
}

namespace qvr {

class CustomComponentEditor;
class CustomComponentType;
class RawInputDevices;

// This type of Component defines custom behaviour for its Entity.
class CustomComponent : public Component {
public:
	CustomComponent(Entity& entity);

	virtual ~CustomComponent();

	CustomComponent(const CustomComponent&) = delete;
	CustomComponent(const CustomComponent&&) = delete;

	CustomComponent& operator=(const CustomComponent&) = delete;
	CustomComponent& operator=(const CustomComponent&&) = delete;

	virtual nlohmann::json ToJson() const { return nlohmann::json(); };
	virtual bool FromJson(const nlohmann::json& j) { return true; }

	// Override this with per-frame behaviour.
	virtual void OnStep(const std::chrono::duration<float> deltaTime) {}

	virtual void HandleInput(
		qvr::RawInputDevices& inputDevices, 
		const std::chrono::duration<float> deltaSeconds) {}

	virtual void OnBeginContact(Entity& other, b2Fixture& myFixture) {}
	virtual void OnEndContact  (Entity& other, b2Fixture& myFixture) {}

	virtual std::unique_ptr<CustomComponentEditor> CreateEditor() { return nullptr; }

	// Override this to return the type name string of your subclass.
	virtual std::string GetTypeName() const = 0;

	bool GetRemoveFlag() const { return mRemoveFlag; }

protected:
	// Signal to the World that this Entity should be removed.
	void SetRemoveFlag(const bool removeFlag) { mRemoveFlag = removeFlag; }

private:
	bool mRemoveFlag = false;
};

class CustomComponentEditor
{
public:
	CustomComponentEditor(CustomComponent& target) : m_Target(target) {};
	virtual ~CustomComponentEditor() {};
	virtual void GuiControls() = 0;
	bool IsTargeting(const CustomComponent& customComponent) const {
		return &customComponent == &m_Target;
	}
protected:
	CustomComponent& m_Target;
};

// Helpful template
template <class T>
class CustomComponentEditorType : public CustomComponentEditor
{
protected:
	T& Target() { return (T&)m_Target; }
public:
	CustomComponentEditorType(T& t) : CustomComponentEditor(t) {}
};

class CustomComponentType final {
public:
	CustomComponentType(
		const std::string typeName,
		std::function<std::unique_ptr<CustomComponent>(Entity&)> factoryFunc);

	CustomComponentType(const CustomComponentType&) = delete;
	CustomComponentType(const CustomComponentType&&) = delete;

	CustomComponentType& operator=(const CustomComponentType&) = delete;
	CustomComponentType& operator=(const CustomComponentType&&) = delete;

	std::unique_ptr<CustomComponent> CreateInstance(Entity& entity) {
		return mFactoryFunc(entity);
	}

	std::unique_ptr<CustomComponent> CreateInstance(Entity& entity, const nlohmann::json& j);

	std::string GetName() const { return mName; };

private:
	std::string mName;
	std::function<std::unique_ptr<CustomComponent>(Entity&)> mFactoryFunc;
};

class CustomComponentTypeLibrary {
public:
	bool RegisterType(std::unique_ptr<CustomComponentType> type);
	bool ForgetType(const std::string typeName);

	bool TypeExists(const std::string typeName) const;

	CustomComponentType* GetType(const std::string typeName) const;

	std::vector<std::string> GetTypeNames() const;

	std::unique_ptr<CustomComponent> CreateInstance(
		Entity& entity,
		const nlohmann::json& j) const;

	bool IsValid(const nlohmann::json& j) const;

private:
	std::unordered_map<std::string, std::unique_ptr<CustomComponentType>> mTypes;
};

}