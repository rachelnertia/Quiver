#pragma once

#include <Quiver/Entity/CustomComponent/CustomComponent.h>

#include "Player.h"

struct PlayerQuiverEditorState {
	int selectedSlot = -1;
};

class PlayerEditor : public qvr::CustomComponentEditorType<Player>
{
public:
	PlayerEditor(Player& player) : CustomComponentEditorType(player) {}

	void GuiControls() override;

private:
	PlayerQuiverEditorState quiverEditorState;

};