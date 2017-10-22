#pragma once

#include <json.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Texture.hpp>

namespace qvr {

class Camera3D;

class Sky {
public:
	class SkyLayer {
	public:
		SkyLayer() : mName("Unnamed Layer")
		{
			SetTextureRepeats(0);
		}

		bool ToJson(nlohmann::json& j) const;
		bool FromJson(const nlohmann::json& j);

		void EditorImGuiControls();

		void Render(sf::RenderTarget& target, const Camera3D& camera) const;

		// WorldEditor-only.
		const char* GetName() const {
			return mName.c_str();
		}

		const char* GetTextureName() const {
			return mTextureName.c_str();
		}

	private:

		bool LoadTexture(const char* filename);

		void SetTextureRepeats(float numRepeats);

		sf::Color mColour1;
		sf::Color mColour2;

		sf::Texture mTexture;

		// WorldEditor-only.
		std::string mName;

		// WorldEditor-only/serialization-only.
		std::string mTextureName;

		float mRepeatsPerCircle = 1.0f;

		float mOffsetRadians = 0;
	};

	bool ToJson(nlohmann::json& j) const;
	bool FromJson(const nlohmann::json& j);

	void EditorImGuiControls();

	void Render(sf::RenderTarget& target, const Camera3D& camera) const;

private:
	bool AddLayer();
	bool RemoveLayer(const int layerIndex);

	std::vector<SkyLayer> mLayers;

	// WorldEditor-only stuff:

	int mSelectedLayerIndex = 0;

};

}