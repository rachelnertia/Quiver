#pragma once

#include <unordered_map>
#include <memory>

namespace sf
{
class SoundBuffer;
}

namespace qvr {

class AudioLibrary
{
public:
	std::shared_ptr<sf::SoundBuffer> LoadSoundBuffer(std::string filename);
private:
	std::unordered_map<std::string, std::weak_ptr<sf::SoundBuffer>> m_LoadedSoundBuffers;
};

}