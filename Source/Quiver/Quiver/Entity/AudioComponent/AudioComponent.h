#pragma once

#include "Quiver/Entity/Component.h"

#include <json.hpp>
#include <SFML/Audio/Sound.hpp>

namespace qvr {

class AudioComponentEditor;
class AudioLibrary;

class AudioComponent : public Component
{
public:
	explicit AudioComponent(Entity& entity);
	explicit AudioComponent(Entity& entity, const nlohmann::json& j);

	~AudioComponent();

	AudioComponent(const AudioComponent&) = delete;
	AudioComponent(const AudioComponent&&) = delete;
	AudioComponent& operator=(const AudioComponent&) = delete;
	AudioComponent& operator=(const AudioComponent&&) = delete;

	nlohmann::json ToJson() const;

	void Update();

	// Set the sound to play. Play won't start until the next call to Update().
	bool SetSound(const std::string filename);
	bool SetSound(const std::string filename, const bool repeat);
	bool SetSound(const std::string filename, const bool repeat, AudioLibrary& audioLibrary);

	void SetPaused(const bool paused);

	void StopSound();

private:
	friend class AudioComponentEditor;

	std::shared_ptr<sf::SoundBuffer> m_SoundBuffer;

	sf::Sound m_Sound;

	bool m_PlayQueued;
};

struct AudioComponentEditorData;

class AudioComponentEditor
{
public:
	AudioComponentEditor(AudioComponent& audioComponent);
	~AudioComponentEditor();
	void GuiControls();
	bool IsTargeting(const AudioComponent& audioComponent) const
	{
		return &audioComponent == &m_AudioComponent;
	}
private:
	AudioComponent& m_AudioComponent;
	std::unique_ptr<AudioComponentEditorData> m_Data;
};

}