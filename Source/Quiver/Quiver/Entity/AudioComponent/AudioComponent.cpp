#include "AudioComponent.h"

#include "ImGui/imgui.h"

#include "Quiver/Audio/AudioLibrary.h"
#include "Quiver/Entity/Entity.h"
#include "Quiver/Entity/PhysicsComponent/PhysicsComponent.h"
#include "Quiver/World/World.h"

namespace qvr {

AudioComponent::AudioComponent(Entity& entity)
	: Component(entity)
{
	GetEntity().GetWorld().RegisterAudioComponent(*this);
}

using json = nlohmann::json;

AudioComponent::AudioComponent(Entity& entity, const json& j)
	: Component(entity)
{
	GetEntity().GetWorld().RegisterAudioComponent(*this);
}

AudioComponent::~AudioComponent()
{
	GetEntity().GetWorld().UnregisterAudioComponent(*this);
}

nlohmann::json AudioComponent::ToJson() const
{
	// TODO

	return nlohmann::json();
}

void AudioComponent::Update()
{
	if (m_PlayQueued)
	{
		m_Sound.play();
		m_PlayQueued = false;
	}

	auto UpdateSoundPosition = [this]()
	{
		if (m_Sound.isRelativeToListener())
		{
			return;
		}

		const b2Vec2 pos = GetEntity().GetPhysics()->GetPosition();
		m_Sound.setPosition(pos.x, 0.0f, pos.y);
	};

	const auto status = m_Sound.getStatus();

	switch (status)
	{
	case sf::SoundSource::Status::Paused:
	case sf::SoundSource::Status::Playing:
		UpdateSoundPosition();
		break;
	case sf::SoundSource::Status::Stopped:
		m_Sound.resetBuffer();
		m_SoundBuffer.reset();
		break;
	}
}

bool AudioComponent::SetSound(const std::string filename, const bool repeat, AudioLibrary& audioLibrary)
{
	m_Sound.resetBuffer();

	m_SoundBuffer = audioLibrary.LoadSoundBuffer(filename);

	if (m_SoundBuffer)
	{
		m_Sound.setBuffer(*m_SoundBuffer.get());

		//m_Sound.setRelativeToListener(true);
		m_Sound.setLoop(repeat);

		m_PlayQueued = true;

		return true;
	}

	return false;
}

bool AudioComponent::SetSound(const std::string filename, const bool repeat)
{
	SetSound(filename, repeat, GetEntity().GetWorld().GetAudioLibrary());

	return false;
}

bool AudioComponent::SetSound(const std::string filename)
{
	return SetSound(filename, false);
}

void AudioComponent::SetPaused(const bool paused)
{
	if (paused)
	{
		m_Sound.pause();
	}
	else
	{
		m_Sound.play();
	}
}

void AudioComponent::StopSound()
{
	m_Sound.resetBuffer();
	m_SoundBuffer.reset();
	m_PlayQueued = false;
}

}