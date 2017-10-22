#include "Sky.h"

#include <algorithm>

#include <ImGui/imgui.h>
#include <SFML/Graphics/RenderTarget.hpp>

#include "Quiver/Graphics/Camera3D.h"
#include "Quiver/Graphics/ColourUtils.h"

namespace qvr {

bool Sky::ToJson(nlohmann::json & j) const
{
	int count = 0;
	for (auto& layer : mLayers) {
		nlohmann::json temp;
		if (layer.ToJson(temp)) {
			j["Layers"][count++] = temp;
		}
	}

	return true;
}

bool Sky::FromJson(const nlohmann::json & j)
{
	static const char* logContext = "Sky::FromJson: ";

	mLayers.clear();

	if (j.find("Layers") != j.end()) {
		if (!j["Layers"].is_array()) {
			std::cout << logContext << "Layers must be an array.\n";
			return false;
		}

		for (auto layerJson : j["Layers"]) {
			mLayers.push_back(SkyLayer());
			if (!mLayers.back().FromJson(layerJson)) {
				mLayers.pop_back();
			}
		}
	}

	return true;
}

void Sky::EditorImGuiControls()
{
	if (mLayers.empty()) {
		ImGui::Text("There are no Sky Layers. The Sky is empty.");
	}
	else {
		auto layerNameGetter = [](void* data, int index, const char** itemText)
		{
			auto& layersArray = *static_cast<std::vector<SkyLayer>*>(data);

			if (index < 0) return false;
			if (index >= (int)layersArray.size()) return false;

			const char* layerName = layersArray[index].GetName();

			*itemText = layerName != nullptr ? layerName : "Unnamed Layer";

			return true;
		};

		if (ImGui::ListBox("Layers", &mSelectedLayerIndex, layerNameGetter,
			&mLayers, mLayers.size()))
		{
			const SkyLayer& selectedSkyLayer = mLayers[mSelectedLayerIndex];
			std::cout << "Selected Sky Layer \"" <<
				(selectedSkyLayer.GetName() != nullptr ? selectedSkyLayer.GetName() : "Unnamed Layer") <<
				"\"\n";
		}

		if (mLayers.size() > 1) {
			if (ImGui::Button("Move Selected Layer up")) {
				if (mSelectedLayerIndex > 0) {
					std::swap(mLayers[mSelectedLayerIndex - 1], mLayers[mSelectedLayerIndex]);
					mSelectedLayerIndex--;
				}
			}
			if (ImGui::Button("Move Selected Layer Down")) {
				if (mSelectedLayerIndex < (int)(mLayers.size() - 1)) {
					std::swap(mLayers[mSelectedLayerIndex + 1], mLayers[mSelectedLayerIndex]);
					mSelectedLayerIndex++;
				}
			}
		}
	}

	if (ImGui::Button("Add New Layer")) {
		if (AddLayer()) {
			std::cout << "Added a new Sky Layer.\n";
		}
		else {
			std::cout << "Could not add a new Sky Layer.\n";
		}
	}

	if (ImGui::Button("Remove Selected Layer")) {
		RemoveLayer(mSelectedLayerIndex);
	}

	if (!mLayers.empty())
	{
		if (ImGui::CollapsingHeader("Modify Selected Layer")) {
			SkyLayer& selectedSkyLayer = mLayers[mSelectedLayerIndex];

			ImGui::Indent();

			selectedSkyLayer.EditorImGuiControls();

			ImGui::Unindent();
		}
	}
}

void Sky::Render(sf::RenderTarget & target, const Camera3D & camera) const
{
	for (auto skyLayer : mLayers) {
		skyLayer.Render(target, camera);
	}
}

bool Sky::AddLayer() {
	mLayers.push_back(SkyLayer());
	mSelectedLayerIndex = mLayers.size() - 1;
	return true;
}

bool Sky::RemoveLayer(const int layerIndex) {
	assert(layerIndex >= 0);
	assert(layerIndex < (int)mLayers.size());

	mLayers.erase(mLayers.begin() + layerIndex);

	// WorldEditor-only:
	// If the removed element was the last one, the new selection is the 
	// new last element.
	mSelectedLayerIndex =
		layerIndex >= (int)mLayers.size() ? (int)mLayers.size() - 1 : layerIndex;

	return true;
}

namespace Keys {
static const char* keyForName = "Name";
static const char* keyForTexture = "Texture";
static const char* keyForRepeats = "RepeatCount";
static const char* keyForOffsetRadians = "OffsetRadians";
static const char* keyForTextureIsRepeating = "TextureIsRepeating";
static const char* keyForColours = "Colours";
}

bool Sky::SkyLayer::ToJson(nlohmann::json & j) const
{
	using namespace Keys;

	j.clear();

	j[keyForName] = mName;

	if (!mTextureName.empty()) {
		j[keyForTexture] = mTextureName;
		j[keyForRepeats] = mRepeatsPerCircle;
		j[keyForOffsetRadians] = mOffsetRadians;
	}

	j[keyForTextureIsRepeating] = mTexture.isRepeated();

	ColourUtils::SerializeSFColorToJson(mColour1, j[keyForColours][0]);
	ColourUtils::SerializeSFColorToJson(mColour2, j[keyForColours][1]);

	return true;
}

bool Sky::SkyLayer::FromJson(const nlohmann::json & j)
{
	using namespace Keys;

	if (j.empty()) { return false; }
	if (!j.is_object()) { return false; }
	if (j.find(keyForName) == j.end()) { return false; }
	if (!j[keyForName].is_string()) { return false; }

	if (j.find(keyForTexture) != j.end()
		&& !j[keyForTexture].is_string())
	{
		// If texture field is present the value must be a string.
		return false;
	}

	if (j.find(keyForTextureIsRepeating) != j.end()
		&& !j[keyForTextureIsRepeating].is_boolean())
	{
		return false;
	}

	if (j.find(keyForOffsetRadians) != j.end()
		&& !j[keyForOffsetRadians].is_number())
	{
		return false;
	}

	if (j.find(keyForRepeats) != j.end()
		&& !j[keyForRepeats].is_number())
	{
		return false;
	}

	if (j.find(keyForColours) != j.end()
		&& !j[keyForColours].is_array())
	{
		return false;
	}

	mName = j[keyForName].get<std::string>();

	// Not a failure if we can't load the texture file?
	if (j.find(keyForTexture) != j.end()) {
		LoadTexture(j[keyForTexture].get<std::string>().c_str());
	}

	if (j.find(keyForRepeats) != j.end()) {
		SetTextureRepeats(j[keyForRepeats].get<float>());
	}

	if (j.find(keyForOffsetRadians) != j.end()) {
		mOffsetRadians = j[keyForOffsetRadians].get<float>();
	}

	if (j.find(keyForTextureIsRepeating) != j.end()) {
		mTexture.setRepeated(j[keyForTextureIsRepeating].get<bool>());
	}

	if (j.find(keyForColours) != j.end()) {
		if (j[keyForColours].size() == 2) {
			ColourUtils::DeserializeSFColorFromJson(mColour1, j[keyForColours][0]);
			ColourUtils::DeserializeSFColorFromJson(mColour2, j[keyForColours][1]);
		}
	}

	return true;
}

bool Sky::SkyLayer::LoadTexture(const char* filename) {
	if (mTexture.loadFromFile(filename)) {
		std::cout << "Loaded texture file \"" << filename << "\".\n";
		mTextureName = filename;
		return true;
	}
	else {
		std::cout << "Could not load texture file \"" << mTextureName.c_str() << "\".\n";
		mTextureName.clear();
	}
	return false;
}

void Sky::SkyLayer::SetTextureRepeats(float numRepeats)
{
	mRepeatsPerCircle = std::fmaxf(1, numRepeats);
	//mTexture.setRepeated(numRepeats > 0);
}

void Sky::SkyLayer::EditorImGuiControls()
{
	ImGui::Text("Layer Name: %s", mName.c_str() != nullptr ? mName.c_str() : "None");

	{
		ImGui::Text("Texture");

		if (!mTextureName.empty()) {
			ImGui::Text("Texture Filename : %s", mTextureName.c_str());

			if (ImGui::Button("Unload")) {
				mTexture = sf::Texture();
				mTextureName.clear();
			}

			if (ImGui::Button("Reload")) {
				LoadTexture(mTextureName.c_str());
			}
		}
		else
		{
			static char buffer[128] = { 0 };

			ImGui::InputText("Texture Filename", buffer, 128);

			if (ImGui::Button("Load")) {
				LoadTexture(buffer);
			}
		}

		{
			float repeats = mRepeatsPerCircle;
			if (ImGui::SliderFloat("Num Repeats", &repeats, 1.0f, 10.0f)) {
				SetTextureRepeats(repeats);
			}
		}

		{
			bool textureIsRepeated = mTexture.isRepeated();
			if (ImGui::Checkbox("Is Repeated", &textureIsRepeated)) {
				mTexture.setRepeated(textureIsRepeated);
			}
		}

		ImGui::SliderAngle("Offset Degrees", &mOffsetRadians, 0, 360);

		ColourUtils::ImGuiColourEdit("Colour##1", mColour1);
		ColourUtils::ImGuiColourEdit("Colour##2", mColour2);
	}
}

void Sky::SkyLayer::Render(sf::RenderTarget & target, const Camera3D & camera) const
{
	const sf::Vector2f targetSize = sf::Vector2f((float)target.getSize().x, (float)target.getSize().y);

	const float tau = b2_pi * 2.0f;

	const float pitchOffset = (float)GetPitchOffsetInPixels(camera, (int)targetSize.y);

	const float top = pitchOffset;
	const float bottom = (targetSize.y / 2.0f) + pitchOffset;

	if (mTexture.isRepeated()) {
		const float texelsPerCircumference = (mTexture.getSize().x / tau) * std::max(1, (int)mRepeatsPerCircle);
		const float rotation = camera.GetRotation() + b2_pi;
		const float angle = fmod(rotation + mOffsetRadians, tau);
		const float offsetTexels = angle * texelsPerCircumference;
		const float halfWidthTexels = texelsPerCircumference * camera.GetViewPlaneWidthModifier();

		const float left = offsetTexels - halfWidthTexels;
		const float right = offsetTexels + halfWidthTexels;

		sf::Vertex verts[4] =
		{
			sf::Vertex(sf::Vector2f(0.0f, top), mColour1, sf::Vector2f(left, 0)),
			sf::Vertex(sf::Vector2f(0.0f, bottom), mColour2, sf::Vector2f(left, (float)mTexture.getSize().y)),

			sf::Vertex(sf::Vector2f(targetSize.x, bottom), mColour2, sf::Vector2f(right, (float)mTexture.getSize().y)),
			sf::Vertex(sf::Vector2f(targetSize.x, top), mColour1, sf::Vector2f(right, 0.0f))
		};

		sf::RenderStates rs;
		rs.texture = &mTexture;

		target.draw(verts, 4, sf::PrimitiveType::Quads, rs);
	}
	else {
		// TODO: At mRepeatsPerCircle == 1, this is broken.

		const float texelsPerCircumference = (mTexture.getSize().x / tau) * std::fmax(1.0f, mRepeatsPerCircle);
		const float rotation = camera.GetRotation() + b2_pi;
		const float angle = rotation + mOffsetRadians;
		const float offsetTexels = fmod(angle * texelsPerCircumference, texelsPerCircumference * tau);
		const float halfWidthTexels = texelsPerCircumference * camera.GetViewPlaneWidthModifier();

		const float offsetTexelsA = offsetTexels;
		const float left = offsetTexelsA - halfWidthTexels;

		const float right = fmod(angle * texelsPerCircumference + halfWidthTexels, texelsPerCircumference * tau);
		const float offsetTexelsB = right - halfWidthTexels;

		sf::Vertex verts[8] =
		{
			sf::Vertex(sf::Vector2f(0.0f, top), mColour1, sf::Vector2f(left, 0)),
			sf::Vertex(sf::Vector2f(0.0f, bottom), mColour2, sf::Vector2f(left, (float)mTexture.getSize().y)),

			sf::Vertex(sf::Vector2f(targetSize.x / 2.0f, bottom), mColour2, sf::Vector2f(offsetTexelsA, (float)mTexture.getSize().y)),
			sf::Vertex(sf::Vector2f(targetSize.x / 2.0f, top), mColour1, sf::Vector2f(offsetTexelsA, 0)),

			sf::Vertex(sf::Vector2f(targetSize.x / 2.0f, top), mColour1, sf::Vector2f(offsetTexelsB, 0)),
			sf::Vertex(sf::Vector2f(targetSize.x / 2.0f, bottom), mColour2, sf::Vector2f(offsetTexelsB, (float)mTexture.getSize().y)),

			sf::Vertex(sf::Vector2f(targetSize.x, bottom), mColour2, sf::Vector2f(right, (float)mTexture.getSize().y)),
			sf::Vertex(sf::Vector2f(targetSize.x, top), mColour1, sf::Vector2f(right, 0.0f))
		};

		sf::RenderStates rs;
		rs.texture = &mTexture;

		target.draw(verts, 8, sf::PrimitiveType::Quads, rs);
	}
}

}