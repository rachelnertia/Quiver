#pragma once

#include "ImGui/imgui.h"

#include <string>

namespace ImGui
{
	struct AutoIndent
	{
		AutoIndent()
		{
			ImGui::Indent();
		}

		~AutoIndent()
		{
			ImGui::Unindent();
		}
	};

	struct AutoWindow
	{
		AutoWindow(const char* name, bool* p_open, ImGuiWindowFlags flags)
		{
			ImGui::Begin(name, p_open, flags);
		}

		~AutoWindow()
		{
			ImGui::End();
		}
	};

	struct AutoStyleVar
	{
		AutoStyleVar(ImGuiStyleVar idx, float val)
		{
			ImGui::PushStyleVar(idx, val);
		}

		AutoStyleVar(ImGuiStyleVar idx, const ImVec2& val)
		{
			ImGui::PushStyleVar(idx, val);
		}

		~AutoStyleVar()
		{
			ImGui::PopStyleVar();
		}
	};

	bool ListBox(const char* label, int* current_item, const std::string* items, const int items_count);

	template<int bufferSize>
	bool InputText(const char* label, char (&str)[bufferSize], ImGuiInputTextFlags flags = 0, ImGuiTextEditCallback callback = nullptr, void* user_data = nullptr)
	{
		return InputText(label, str, bufferSize, flags, callback, user_data);
	}

	template<int bufferSize>
	bool InputText(const char* label, std::string& str, ImGuiInputTextFlags flags = 0, ImGuiTextEditCallback callback = nullptr, void* user_data = nullptr)
	{
		char buffer[bufferSize];
		
		if (str.empty()) {
			buffer[0] = '\0';
		}
		else {
			strcpy(buffer, str.c_str());
		}

		const bool ret = ImGui::InputText(label, buffer, bufferSize, flags, callback, user_data);

		if (ret) {
			str = buffer;
		}

		return ret;
	}
}
