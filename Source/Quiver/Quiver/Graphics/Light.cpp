#include "Light.h"

#include <ImGui/imgui.h>

#include "Quiver/Graphics/ColourUtils.h"

namespace qvr {

using json = nlohmann::json;

json ToJson(const AmbientLight & light)
{
	json j;

	ColourUtils::SerializeSFColorToJson(light.mColor, j["Colour"]);

	return j;
}

AmbientLight FromJson(const json& j)
{
	AmbientLight d;

	if (!j.empty())
	{
		ColourUtils::DeserializeSFColorFromJson(d.mColor, j.value<json>("Colour", {}));
	}

	return d;
}

bool DirectionalLight::ToJson(nlohmann::json & j) const
{
	if (!j.empty()) {
		return false;
	}

	// Store direction as an angle.
	j["Angle"] = atan2f(mDirection.y, mDirection.x);

	return ColourUtils::SerializeSFColorToJson(mColor, j["Color"]);
}

bool DirectionalLight::FromJson(const nlohmann::json & j)
{
	if (!j.is_object()) {
		return false;
	}

	if (j.find("Angle") != j.end()) {
		SetDirection(j["Angle"].get<float>());
	}

	if (j.find("Color") != j.end()) {
		sf::Color c;
		bool r = ColourUtils::DeserializeSFColorFromJson(c, j["Color"]);
		if (!r) return false;
		SetColor(c);
	}

	return true;
}

void DirectionalLight::GuiControls()
{
	float angle = atan2f(mDirection.y, mDirection.x);
	if (ImGui::SliderAngle("Angle", &angle, -180.0f, 180.0f)) {
		SetDirection(angle);
	}

	ColourUtils::ImGuiColourEditRGB("Colour##DirectionalLight", mColor);
}

}