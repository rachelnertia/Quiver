#include "AudioLibrary.h"

#include <SFML/Audio/SoundBuffer.hpp>

#include <spdlog/spdlog.h>

namespace qvr {

std::shared_ptr<sf::SoundBuffer> AudioLibrary::LoadSoundBuffer(std::string filename)
{
	const char* logCtx = "AudioLibrary::LoadSoundBuffer";
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
	if (m_LoadedSoundBuffers.find(filename) != m_LoadedSoundBuffers.end())
	{
		// Check if the weak pointer is dangling.
		if (m_LoadedSoundBuffers[filename].expired()) {
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
				m_LoadedSoundBuffers[filename].use_count());
			// Create a new reference to the asset.
			return m_LoadedSoundBuffers[filename].lock();
		}
	}

	// Need to try loading.
	std::unique_ptr<sf::SoundBuffer> temp = std::make_unique<sf::SoundBuffer>();

	if (temp->loadFromFile(filename)) {
		log->debug(
			"{}: {} was loaded successfully.",
			logCtx,
			filename.c_str());
		// Load was successful.
		std::shared_ptr<sf::SoundBuffer> shared(temp.release());
		// Keep track of it.
		m_LoadedSoundBuffers[filename] = shared;
		return shared;
	}

	log->debug("{}: Failed to load {}.", logCtx, filename.c_str());

	return nullptr;
}

}