#include "ImGuiHelpers.h"

namespace ImGui
{
	bool ListBox(const char* label, int* current_item, const std::string* items, const int items_count)
	{
		const auto items_getter = [](void* data, int idx, const char** out_text) -> bool
		{
			const auto strArray = (std::string*)data;

			*out_text = strArray[idx].c_str();

			return true;
		};

		return ImGui::ListBox(label, current_item, items_getter, (void*)items, items_count);
	}
}