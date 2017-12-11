#include "TextureLibrary.h"

#include <algorithm>

#include <ImGui/imgui.h>
#include <ImGui/imgui-SFML.h>
#include <SFML/Graphics/Texture.hpp>
#include <spdlog/spdlog.h>

#include "Quiver/Misc/ImGuiHelpers.h"

namespace qvr {

std::shared_ptr<sf::Texture> TextureLibrary::LoadTexture(std::string filename)
{
	const char* logCtx = "TextureLibrary::LoadTexture";
	auto log = spdlog::get("console");
	assert(log);

	// Need to cast filename to all-lower case.
	std::transform(
		filename.begin(),
		filename.end(),
		filename.begin(),
		[](const char c) -> char
	{
		return static_cast<char>(std::tolower(static_cast<int>(c)));
	});

	// Check if there's already a copy of it in memory.
	if (mLoadedTextures.find(filename) != mLoadedTextures.end())
	{
		// Check if the weak pointer is dangling.
		if (mLoadedTextures[filename].expired()) {
			log->debug(
				"{}: {} was loaded before, but at present has 0 references. "
				"Need to load it.",
				logCtx,
				filename.c_str());
		}
		else {
			log->debug(
				"{}: {} is already loaded and has {} references."
				"Making a new reference.",
				logCtx,
				filename.c_str(),
				mLoadedTextures[filename].use_count());
			// Create a new reference to the asset.
			return mLoadedTextures[filename].lock();
		}
	}

	// Need to try loading.
	auto texture = std::make_shared<sf::Texture>();

	if (texture->loadFromFile(filename)) {
		log->debug(
			"{}: {} was loaded successfully.",
			logCtx,
			filename.c_str());
		// Keep track of it.
		mLoadedTextures[filename] = texture;
		return texture;
	}

	log->debug("{}: Failed to load {}.", logCtx, filename.c_str());

	return nullptr;
}

void TextureLibraryGui::ProcessGui() {
	using namespace std;

	if (mTextureLibrary.mLoadedTextures.empty()) {
		ImGui::Text("No Textures");
		
		return;
	}

	vector<string> textureNames;
	for (const auto& kvp : mTextureLibrary.mLoadedTextures) {
		textureNames.push_back(kvp.first);
	}

	int index = -1;
	if (!mCurrentTextureName.empty()) {
		for (int i = 0; i < (int)textureNames.size(); i++) {
			if (textureNames[i] == mCurrentTextureName) {
				index = i;
				break;
			}
		}
		if (index < 0) {
			mCurrentTextureName.clear();
		}
	}

	if (ImGui::ListBox("Textures", &index, &textureNames[0], (int)textureNames.size())) {
		mCurrentTextureName = textureNames[index];
	}

	if (!mCurrentTextureName.empty()) {
		const auto& tex = mTextureLibrary.mLoadedTextures.at(mCurrentTextureName);

		ImGui::Text("Loaded: %s", tex.expired() ? "False" : "True");

		if (!tex.expired()) {
			const auto loadedTex = tex.lock();
			ImGui::Image(*loadedTex);
		}
	}
}

}