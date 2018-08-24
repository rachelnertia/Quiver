#include "PlayerEditor.h"

#include <Quiver/Misc/ImGuiHelpers.h>

void ImGuiControls(DamageCount& damageCounter, const int damageMaximumLimit) {
	ImGui::SliderInt("Damage Taken",   &damageCounter.damage, 0, damageCounter.max);
	ImGui::SliderInt("Damage Maximum", &damageCounter.max,    0, damageMaximumLimit);
}

void PlayerEditor::GuiControls() {
	ImGui::SliderFloat(
		"Move Speed", 
		Target().mMoveSpeed, 
		&MovementSpeed::GetBase, 
		&MovementSpeed::SetBase, 
		0.0f, 
		20.0f);

	if (ImGui::CollapsingHeader("Damage##PlayerDamage")) {
		ImGui::Checkbox("Cannot Die", &Target().mCannotDie);
		ImGuiControls(Target().mDamage, 100);
	}
}
