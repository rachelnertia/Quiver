#include <Quiver/Misc/ImGuiHelpers.h>
#include <Quiver/Entity/CustomComponent/CustomComponent.h>

#include "Player.h"

using namespace qvr;

struct PlayerQuiverEditorState {
	int selectedSlot = -1;
};

class PlayerEditor : public CustomComponentEditorType<Player>
{
public:
	PlayerEditor(Player& player) : CustomComponentEditorType(player) {}

	void GuiControls() override;

private:
	PlayerQuiverEditorState quiverEditorState;

};

std::unique_ptr<CustomComponentEditor> Player::CreateEditor() {
	return std::make_unique<PlayerEditor>(*this);
}

void ImGuiControls(DamageCount& damageCounter, const int damageMaximumLimit) {
	ImGui::SliderInt("Damage Taken",   &damageCounter.damage, 0, damageCounter.max);
	ImGui::SliderInt("Damage Maximum", &damageCounter.max,    0, damageMaximumLimit);
}

void ImGuiControls(
	MovementSpeed& movementSpeed, 
	const float baseLimit,
	const float multiplierLimit) 
{
	ImGui::Text("Move Speed: %.2f", movementSpeed.Get());

	ImGui::SliderFloat(
		"Move Speed Base",
		movementSpeed,
		&MovementSpeed::GetBase,
		&MovementSpeed::SetBase,
		0.0f,
		baseLimit);

	ImGui::SliderFloat(
		"Move Speed Multiplier",
		movementSpeed,
		&MovementSpeed::GetMultiplier,
		&MovementSpeed::SetMultiplier,
		0.0f,
		multiplierLimit);
	
	if (ImGui::Button("Reset Multiplier")) {
		movementSpeed.ResetMultiplier();
	}
}

void ImGuiControls(PlayerQuiver& quiver, PlayerQuiverEditorState& state) {
	
	auto itemsGetter = [](void* data, int index, const char** itemText) -> bool
	{
		auto slots = (PlayerQuiver::Slots*)data;

		if (index >= (int)slots->size()) return false;
		if (index < 0) return false;

		if (slots->at(index).has_value()) {
			*itemText = "Quarrel";
		}
		else {
			*itemText = "Empty";
		}

		return true;
	};

	ImGui::ListBox(
		"Slots##PlayerQuiver", 
		&state.selectedSlot,
		itemsGetter, 
		(void*)&quiver.quarrelSlots, 
		PlayerQuiver::MaxEquippedQuarrelTypes);
}

void PlayerEditor::GuiControls() {
	if (ImGui::CollapsingHeader("Movement##PlayerEditor")) {
		ImGuiControls(Target().mMoveSpeed, 20.0f, 3.0f);
	}

	if (ImGui::CollapsingHeader("Damage##PlayerEditor")) {
		ImGui::Checkbox("Cannot Die", &Target().mCannotDie);
		ImGuiControls(Target().mDamage, 100);
	}

	if (ImGui::CollapsingHeader("Quiver##PlayerEditor")) {
		ImGuiControls(Target().quiver, quiverEditorState);
	}
}
