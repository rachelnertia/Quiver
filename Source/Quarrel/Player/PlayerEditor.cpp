#include "PlayerEditor.h"

#include <Quiver/Misc/ImGuiHelpers.h>

void PlayerEditor::GuiControls() {
	ImGui::SliderFloat("Move Speed", Target().mMoveSpeed, &MovementSpeed::GetBase, &MovementSpeed::SetBase, 0.0f, 20.0f);
}
