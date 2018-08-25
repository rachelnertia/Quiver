#include <Quiver/Graphics/ColourUtils.h>
#include <Quiver/Misc/ImGuiHelpers.h>
#include <Quiver/Entity/CustomComponent/CustomComponent.h>

#include "Player.h"
#include "Misc/Utils.h"

using namespace qvr;

struct PlayerQuiverEditorState {
	int selectedSlot = -1;
};

struct PlayerQuarrelLibraryEditorState {
	int selectedSlot = -1;
};

class PlayerEditor : public CustomComponentEditorType<Player>
{
public:
	PlayerEditor(Player& player) : CustomComponentEditorType(player) {}

	void GuiControls() override;

private:
	PlayerQuiverEditorState quiverEditorState;
	PlayerQuarrelLibraryEditorState quarrelLibraryEditorState;
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
	
	float seconds = quarrelType.cooldownTime.count();
	ImGui::InputFloat("Cooldown Time", &seconds);
	quarrelType.cooldownTime = std::chrono::duration<float>(seconds);
	
	if (ImGui::CollapsingHeader("Effects")) {
		ImGui::AutoIndent indent;
		ImGuiControls(quarrelType.effect);
	}
}

bool QuarrelLibraryListBox(const PlayerQuarrelLibrary& library, int& selection) {
	auto itemsGetter = [](void* data, int index, const char** itemText)->bool
	{
		auto list = (PlayerQuarrelLibrary::QuarrelList*)data;

		if (index >= (int)list->size()) return false;
		if (index < 0) return false;

		*itemText = list->at(index).name.c_str();

		return true;
	};

	return ImGui::ListBox(
		"Library", 
		&selection, 
		itemsGetter, 
		(void*)&library.quarrels, 
		library.quarrels.size());
}

void ImGuiControls(
	PlayerQuiver& quiver, 
	PlayerQuiverEditorState& state, 
	PlayerQuarrelLibrary& library) 
{	
	ImGui::AutoID id(&quiver);

	auto itemsGetter = [](void* data, int index, const char** itemText) -> bool
	{
		auto slots = (PlayerQuiver::Slots*)data;

		if (index >= (int)slots->size()) return false;
		if (index < 0) return false;

		if (slots->at(index).has_value()) {
			*itemText = slots->at(index)->type.name.c_str();
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
				
				if (!quarrelType.name.empty() &&
					ImGui::Button("Put Type in Library"))
				{
					auto& name = quarrelType.name;

					AddOrUpdate(
						library.quarrels, 
						quarrelType, 
						[&name](auto& quarrelType)
						{
							return quarrelType.name == name;
						});
				}
			}
		}
		else {
			ImGui::Text("This slot is empty");

			if (ImGui::CollapsingHeader("Take Type from Library")) {
				int selection = -1;

				if (QuarrelLibraryListBox(library, selection)) {
					quiver.quarrelSlots[state.selectedSlot].emplace(
						library.quarrels[selection]);
				}
			}
		}
	}
}

void ImGuiControls(
	PlayerQuarrelLibrary& library, 
	PlayerQuarrelLibraryEditorState& editorState)
{
	ImGui::AutoID id(&library);

	QuarrelLibraryListBox(library, editorState.selectedSlot);

	if (editorState.selectedSlot < 0 ||
		editorState.selectedSlot > (int)library.quarrels.size())
	{
		return;
	}

	auto& quarrelType = library.quarrels[editorState.selectedSlot];

	ImGuiControls(quarrelType);
}

void PlayerEditor::GuiControls() {
	ImGui::AutoID id(this);

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

	if (ImGui::CollapsingHeader("Quarrel Library")) {
		ImGuiControls(Target().quarrelLibrary, quarrelLibraryEditorState);
	}
}
