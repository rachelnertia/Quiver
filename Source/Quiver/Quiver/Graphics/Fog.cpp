#include "Fog.h"

#include <ImGui/imgui.h>

#include "Quiver/Graphics/ColourUtils.h"

namespace qvr {

void GuiControls(Fog& fog) {
	{
		sf::Color c = fog.GetColor();
		ColourUtils::ImGuiColourEditRGB("Colour##Fog", c);
		fog.SetColor(c);
	}
	{
		float distance = fog.GetMinDistance();
		ImGui::SliderFloat("Minimum Distance", &distance, 0.0f, fog.GetMaxDistance());
		fog.SetMinDistance(distance);
	}
	{
		float distance = fog.GetMaxDistance();
		ImGui::SliderFloat("Maximum Distance", &distance, fog.GetMinDistance(), 100);
		fog.SetMaxDistance(distance);
	}
	{
		float intensity = fog.GetMaxIntensity();
		ImGui::SliderFloat("Maximum Intensity", &intensity, 0.0f, 1.0f);
		fog.SetMaxIntensity(intensity);
	}
}

using json = nlohmann::json;

json ToJson(const Fog& fog)
{
	json j;

	ColourUtils::SerializeSFColorToJson(fog.GetColor(), j["Color"]);

	j["MaxDistance"] = fog.GetMaxDistance();
	j["MinDistance"] = fog.GetMinDistance();
	j["MaxIntensity"] = fog.GetMaxIntensity();

	return j;
}

Fog Fog::FromJson(const json& j)
{
	Fog fog;

	if (j.is_object() && !j.empty()) {
		sf::Color c;
		if (j.count("Color") != 0) {
			ColourUtils::DeserializeSFColorFromJson(c, j["Color"]);
		}
		fog.SetColor(c);
		fog.SetMaxDistance(j.value<float>("MaxDistance", fog.GetMaxDistance()));
		fog.SetMinDistance(j.value<float>("MinDistance", fog.GetMinDistance()));
		fog.SetMaxIntensity(j.value<float>("MaxIntensity", fog.GetMaxIntensity()));
	}

	return fog;
}

}