#include <Quiver/Graphics/ColourUtils.h>
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

void ImGuiControls(ActiveEffectType& activeEffect) {
	ImGui::ListBox(
		"Active Effect to Apply",
		&activeEffect._value,
		ActiveEffectType::_names().begin(),
		ActiveEffectType::_names().size());
}

void ImGuiControls(SpecialEffectType& specialEffect) {
	ImGui::ListBox(
		"Special Effect",
		&specialEffect._value,
		SpecialEffectType::_names().begin(),
		SpecialEffectType::_names().size());
}

void ImGuiControls(CrossbowBoltEffect& boltEffect) {
	ImGui::AutoID id(&boltEffect);

	ImGui::InputInt("Immediate Damage", &boltEffect.immediateDamage);
	ImGuiControls(boltEffect.appliesEffect);
	ImGuiControls(boltEffect.specialEffect);
}

void ImGuiControls(QuarrelTypeInfo& quarrelType) {
	ImGui::InputText<64>("Name", quarrelType.name);
	ColourUtils::ImGuiColourEditRGB("Colour##QuarrelType", quarrelType.colour);
	ImGuiControls(quarrelType.effect);
}

void ImGuiControls(
	PlayerQuiver& quiver, 
	PlayerQuiverEditorState& state, 
	PlayerQuarrelLibrary& library) 
{	
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

	if (state.selectedSlot >= 0) {
		if (state.selectedSlot > 0 &&
			ImGui::Button("Move Up"))
		{
			std::swap(
				quiver.quarrelSlots[state.selectedSlot - 1],
				quiver.quarrelSlots[state.selectedSlot]);
		}

		const bool shouldSameLine = state.selectedSlot > 0;

		auto doNothing = [](){};

		if (state.selectedSlot < PlayerQuiver::MaxEquippedQuarrelTypes - 1 &&
			(shouldSameLine ? ImGui::SameLine() : doNothing(), ImGui::Button("Move Down")))
		{
			std::swap(
				quiver.quarrelSlots[state.selectedSlot + 1],
				quiver.quarrelSlots[state.selectedSlot]);
		}

		if (quiver.quarrelSlots[state.selectedSlot]) {
			if (ImGui::SameLine(), ImGui::Button("Clear Slot")) {
				quiver.quarrelSlots[state.selectedSlot].reset();
			}
			else {
				auto& quarrelType = (*quiver.quarrelSlots[state.selectedSlot]).type;
				ImGuiControls(quarrelType);
			}
		}
		else {
			ImGui::Text("This slot is empty");
		}
	}
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
		ImGuiControls(Target().quiver, quiverEditorState, Target().quarrelLibrary);
	}
}
