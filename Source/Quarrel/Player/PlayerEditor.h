#pragma once

#include <Quiver/Entity/CustomComponent/CustomComponent.h>

#include "Player.h"

class PlayerEditor : public qvr::CustomComponentEditorType<Player>
{
public:
	PlayerEditor(Player& player) : CustomComponentEditorType(player) {}

	void GuiControls() override;
};