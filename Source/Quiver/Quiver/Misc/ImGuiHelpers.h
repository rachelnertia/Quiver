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

	/*
	Example:
		ImGui::SliderFloat(
			"Friction",
			fixture,
			&b2Fixture::GetFriction,
			&b2Fixture::SetFriction,
			0.0f,
			1.0f);
	*/
	template <typename T>
	bool SliderFloat(
		const char* label,
		T& t,
		float (T::*Getter)() const,
		void (T::*Setter)(float),
		const float min, 
		const float max)
	{
		float f = (t.*Getter)();
		if (ImGui::SliderFloat(label, &f, min, max))
		{
			(t.*Setter)(f);
			return true;
		}
		return false;
	}

	template <typename T>
	bool InputFloat(
		const char* label,
		T& t,
		float (T::*Getter)() const,
		void (T::*Setter)(float))
	{
		float f = (t.*Getter)();
		if (ImGui::InputFloat(label, &f))
		{
			(t.*Setter)(f);
			return true;
		}
		return false;
	}

	template <typename T>
	bool Checkbox(
		const char* label,
		T& t,
		bool (T::*Getter)() const,
		void (T::*Setter)(bool))
	{
		bool b = (t.*Getter)();
		if (ImGui::Checkbox(label, &b))
		{
			(t.*Setter)(b);
			return true;
		}
		return false;
	}
}
