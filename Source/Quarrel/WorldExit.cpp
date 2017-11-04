#include "WorldExit.h"

#include <cstring>

#include <json.hpp>
#include <ImGui/imgui.h>

#include "Quiver/Entity/CustomComponent/CustomComponent.h"
#include "Quiver/Entity/Entity.h"
#include "Quiver/World/World.h"
#include "Quiver/World/WorldContext.h"

using json = nlohmann::json;

using namespace qvr;

class WorldExit : public CustomComponent
{
public:
	WorldExit(Entity& entity);

	void OnBeginContact(Entity& other) override;
	void OnEndContact  (Entity& other)   override;

	std::string GetTypeName() const override { return "WorldExit"; }

	void GUIControls() override;

	json ToJson  () const override;
	bool FromJson(const json& j) override;

	static bool VerifyJson(const json& j);

private:
	// Path to the JSON file containing the World data this WorldExit leads to.
	std::string mWorldFilename;

	//std::unique_ptr<World> mWorld;

	// TODO: Move this into an object that only gets created to go along with WorldExit 
	// instances when in WorldEditor mode / other places where GuiControls can be called.
	std::array<char, 64> mFilenameBuffer;
};

WorldExit::WorldExit(Entity& entity)
	: CustomComponent(entity)
{}

void WorldExit::OnBeginContact(Entity& other)
{
	if (other.GetCustomComponent() && other.GetCustomComponent()->GetTypeName() == "Player") {
		auto world = LoadWorld(mWorldFilename, GetEntity().GetWorld().GetCustomComponentTypes());
		if (world) {
			GetEntity().GetWorld().GetContext().SetNextWorld(std::move(world));
		}
	}
}

void WorldExit::OnEndContact(Entity& other)
{

}

void WorldExit::GUIControls()
{
	const ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue;
	if (ImGui::InputText("World File (JSON)", &mFilenameBuffer[0], mFilenameBuffer.size(), flags))
	{
		mWorldFilename.assign(&mFilenameBuffer[0]);
	}
}

json WorldExit::ToJson() const
{
	json j;
	j["WorldFile"] = mWorldFilename;
	return j;
}

bool WorldExit::FromJson(const json& j)
{
	mWorldFilename = j.value<std::string>("WorldFile", std::string());

	std::cout << "World File: " << mWorldFilename << std::endl;

	// Set up the WorldEditor-only filename buffer.
	mFilenameBuffer[0] = '\0';

	if (!mWorldFilename.empty())
	{
		std::strcpy(mFilenameBuffer.data(), mWorldFilename.c_str());
	}

	return true;
}

bool WorldExit::VerifyJson(const json& j)
{
	return j.count("WorldFile") > 0 && j["WorldFile"].is_string();
}

std::unique_ptr<CustomComponent> CreateWorldExit(Entity& entity)
{
	return std::make_unique<WorldExit>(entity);
}

bool VerifyWorldExitJson(const json& j)
{
	return WorldExit::VerifyJson(j);
}
