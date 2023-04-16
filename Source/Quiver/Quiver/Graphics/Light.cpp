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

void DirectionalLight::SetAngleHorizontal(const float lightAngleRadians) {
	mDirection.x = cosf(lightAngleRadians) * (1.0f - mDirection.z);
	mDirection.y = sinf(lightAngleRadians) * (1.0f - mDirection.z);
	mDirection.Normalize();
}

void DirectionalLight::SetAngleVertical(const float lightAngleRadians) {
	mDirection.x = cosf(lightAngleRadians) * (1.0f - mDirection.y);
	mDirection.z = sinf(lightAngleRadians) * (1.0f - mDirection.y);
	mDirection.Normalize();
}

bool DirectionalLight::ToJson(nlohmann::json & j) const
{
	if (!j.empty()) {
		return false;
	}

	// Store direction as an angle.
	j["AngleHorizontal"] = atan2f(mDirection.y, mDirection.x);
	j["AngleVertical"] = atan2f(mDirection.z, mDirection.x);

	return ColourUtils::SerializeSFColorToJson(mColor, j["Color"]);
}

bool DirectionalLight::FromJson(const nlohmann::json & j)
{
	if (!j.is_object()) {
		return false;
	}

	if (j.find("Angle") != j.end()) {
		SetAngleHorizontal(j["Angle"].get<float>());
	}

	if (j.find("AngleHorizontal") != j.end()) {
		SetAngleHorizontal(j["AngleHorizontal"].get<float>());
	}
	
	if (j.find("AngleVertical") != j.end())
	{
		SetAngleVertical(j["AngleVertical"].get<float>());
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
	ImGui::PushID(this);

	if (ImGui::SliderFloat3("Direction (XYZ)", &mDirection.x, -1.0f, 1.0f))
	{
		mDirection.Normalize();
	}

	ColourUtils::ImGuiColourEditRGB("Colour##DirectionalLight", mColor);

	ImGui::PopID();
}

}