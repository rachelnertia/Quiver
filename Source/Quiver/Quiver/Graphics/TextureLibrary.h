#pragma once

#include <memory>
#include <unordered_map>

namespace sf {
	class Texture;
}

namespace qvr {

class TextureLibrary
{
public:
	std::shared_ptr<sf::Texture> LoadTexture(std::string filename);
private:
	std::unordered_map<std::string, std::weak_ptr<sf::Texture>> mLoadedTextures;

	friend class TextureLibraryGui;
};

class TextureLibraryGui
{
public:
	TextureLibraryGui(TextureLibrary& library) : mTextureLibrary(library) {}
	void ProcessGui();
private:
	TextureLibrary& mTextureLibrary;
	std::string mCurrentTextureName;
};

}